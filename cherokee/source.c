/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "common-internal.h"
#include "source.h"
#include "config_node.h"
#include "resolv_cache.h"
#include "util.h"
#include "thread.h"
#include "connection-protected.h"
#include "bogotime.h"

#define ENTRIES "source,src"

typedef struct {
	cherokee_list_t list;
	cherokee_socket_t socket;
	cherokee_source_protocol_t protocol;
	uint64_t born, idle_since;
	cuint_t requests;
	cushort_t next_id;
} source_idle_t;

typedef struct {
	cherokee_list_t idle;
	CHEROKEE_MUTEX_T(mutex);
	cuint_t count;
} source_pool_t;

cherokee_boolean_t
cherokee_source_socket_clean (cherokee_socket_t *socket)
{
	char byte;
	ssize_t n;
	if (socket->socket < 0 || socket->is_tls != non_TLS) return false;
	do { n = recv(socket->socket, &byte, 1, MSG_PEEK | MSG_DONTWAIT); } while (n < 0 && errno == EINTR);
	/* Data, EOF and errors all invalidate an idle connection. Never drain
	 * unexpected data: it may belong to an incomplete previous response. */
	return n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK);
}

static void
idle_free (source_pool_t *pool, source_idle_t *idle)
{
	cherokee_list_del(&idle->list);
	pool->count--;
	cherokee_socket_close(&idle->socket);
	cherokee_socket_mrproper(&idle->socket);
	free(idle);
}

static void
pool_expire (cherokee_source_t *src, source_pool_t *pool, cherokee_boolean_t check_health)
{
	cherokee_list_t *i, *next;
	uint64_t now = cherokee_monotonic_msec();
	list_for_each_safe(i, next, &pool->idle) {
		source_idle_t *idle = (source_idle_t *)i;
		if (now - idle->idle_since >= (uint64_t)src->keepalive_timeout * 1000 ||
		    now - idle->born >= (uint64_t)src->keepalive_age * 1000 ||
		    (check_health && !cherokee_source_socket_clean(&idle->socket))) idle_free(pool, idle);
	}
}

ret_t
cherokee_source_reuse_cleanup (cherokee_buffer_t *key, void *value, void *param)
{
	cherokee_source_t *src = value;
	source_pool_t *pool = src->reuse_pool;
	UNUSED(key); UNUSED(param);
	CHEROKEE_MUTEX_LOCK(&pool->mutex);
	pool_expire(src, pool, true);
	CHEROKEE_MUTEX_UNLOCK(&pool->mutex);
	return ret_ok;
}

ret_t
cherokee_source_reuse_get (cherokee_source_t *src, cherokee_source_protocol_t protocol,
                           cherokee_socket_t *socket, uint64_t *born, cuint_t *requests, cushort_t *id)
{
	source_pool_t *pool = src->reuse_pool;
	cherokee_list_t *i, *next;
	ret_t ret = ret_not_found;
	*born = cherokee_monotonic_msec(); *requests = 0; *id = 1;
	if (src->keepalive == 0 || (src->keepalive < 0 && protocol == source_protocol_fcgi)) return ret;
	CHEROKEE_MUTEX_LOCK(&pool->mutex);
	pool_expire(src, pool, false);
	list_for_each_safe(i, next, &pool->idle) {
		source_idle_t *idle = (source_idle_t *)i;
		if (idle->protocol != protocol) continue;
		if (!cherokee_source_socket_clean(&idle->socket)) { idle_free(pool, idle); continue; }
		*socket = idle->socket; *born = idle->born; *requests = idle->requests; *id = idle->next_id;
		cherokee_list_del(i); pool->count--; free(idle);
		ret = ret_ok;
		break;
	}
	CHEROKEE_MUTEX_UNLOCK(&pool->mutex);
	return ret;
}

ret_t
cherokee_source_reuse_put (cherokee_source_t *src, cherokee_source_protocol_t protocol,
                           cherokee_socket_t *socket, uint64_t born, cuint_t requests,
                           cushort_t next_id, cuint_t limit)
{
	source_pool_t *pool = src->reuse_pool;
	source_idle_t *idle;
	if (src->keepalive == 0 || (src->keepalive < 0 && protocol == source_protocol_fcgi) ||
	    !limit || requests >= src->keepalive_requests || !next_id ||
	    cherokee_monotonic_msec() - born >= (uint64_t)src->keepalive_age * 1000 || !cherokee_source_socket_clean(socket)) return ret_deny;
	idle = malloc(sizeof(*idle));
	if (!idle) return ret_nomem;
	CHEROKEE_MUTEX_LOCK(&pool->mutex);
	pool_expire(src, pool, false);
	if (limit > src->keepalive_max) limit = src->keepalive_max;
	if (pool->count >= limit) {
		CHEROKEE_MUTEX_UNLOCK(&pool->mutex); free(idle); return ret_deny;
	}
	idle->socket = *socket; idle->protocol = protocol; idle->born = born;
	idle->requests = requests; idle->next_id = next_id; idle->idle_since = cherokee_monotonic_msec();
	cherokee_list_add(&idle->list, &pool->idle); pool->count++;
	/* Transfer ownership. No parser buffers or application data enter pool. */
	cherokee_socket_init(socket);
	CHEROKEE_MUTEX_UNLOCK(&pool->mutex);
	return ret_ok;
}


/* Implements _new() and _free()
 */
CHEROKEE_ADD_FUNC_NEW  (source);
CHEROKEE_ADD_FUNC_FREE (source);


ret_t
cherokee_source_init (cherokee_source_t *src)
{
	cherokee_buffer_init (&src->original);
	cherokee_buffer_init (&src->unix_socket);
	cherokee_buffer_init (&src->host);

	src->type         = source_host;
	src->port         = -1;
	src->free         = NULL;
	src->keepalive = -1;
	src->keepalive_max = 8;
	src->keepalive_timeout = 15;
	src->keepalive_requests = 1000;
	src->keepalive_age = 300;
	src->connect_timeout = 0;
	src->io_timeout = 0;
	src->retries = 1;
	src->reuse_pool = calloc(1, sizeof(source_pool_t));
	if (!src->reuse_pool) return ret_nomem;
	INIT_LIST_HEAD(&((source_pool_t *)src->reuse_pool)->idle);
	CHEROKEE_MUTEX_INIT(&((source_pool_t *)src->reuse_pool)->mutex, CHEROKEE_MUTEX_FAST);

	return ret_ok;
}


ret_t
cherokee_source_mrproper (cherokee_source_t *src)
{
	if (src->free) {
		src->free (src);
	}

	if (src->reuse_pool) {
		source_pool_t *pool = src->reuse_pool;
		while (!cherokee_list_empty(&pool->idle)) idle_free(pool, (source_idle_t *)pool->idle.next);
		CHEROKEE_MUTEX_DESTROY(&pool->mutex);
		free(pool);
	}
	cherokee_buffer_mrproper (&src->original);
	cherokee_buffer_mrproper (&src->unix_socket);
	cherokee_buffer_mrproper (&src->host);

	return ret_ok;
}


ret_t
cherokee_source_connect (cherokee_source_t *src, cherokee_socket_t *sock)
{
	ret_t                    ret;
	cherokee_resolv_cache_t *resolv;

	/* Short path: it's already connecting
	 */
	if (sock->socket >= 0) {
		ret = cherokee_socket_connect (sock);
		if (ret != ret_deny || !sock->source_addr_current || sock->source_addr_current->ai_next == NULL)
			return ret;
		/* Fall through and attempt to try another address... */
		cherokee_socket_close(sock);
		sock->source_addr_current = sock->source_addr_current->ai_next;
	}

long_path:
	/* Create the new socket and set the target IP info
	 */
	if (! cherokee_buffer_is_empty (&src->unix_socket)) {

		/* Create the socket descriptor
		 */
		ret = cherokee_socket_create_fd (sock, AF_UNIX);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}

		ret = cherokee_socket_gethostbyname (sock, &src->unix_socket);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}
	} else {
		cherokee_boolean_t     tested_all;
		const struct addrinfo *addr;
		const struct addrinfo *addr_info = NULL;

		/* Query the resolv cache
		 */
		ret = cherokee_resolv_cache_get_default (&resolv);
		if (unlikely (ret!=ret_ok)) {
			return ret;
		}

		ret = cherokee_resolv_cache_get_addrinfo (resolv, &src->host, &addr_info);
		if ((ret != ret_ok) || (addr_info == NULL)) {
			return ret_error;
		}

		/* Current address
		 */
		if (sock->source_addr_current) {
			tested_all = false;
			addr = sock->source_addr_current;
		} else {
			tested_all = true;
			addr = addr_info;
		}

		/* Create the fd for the address family
		 *
		 * Iterates through the different addresses of the
		 * host and stores a pointer to the first one with
		 * a supported family.
		 */
		while (addr != NULL) {
			ret = cherokee_socket_create_fd (sock, addr->ai_family);

#ifdef TRACE_ENABLED
			if (cherokee_trace_is_tracing()) {
				ret_t ret2;
				char ip[46];

				ret2 = cherokee_ntop (addr->ai_family, addr->ai_addr, ip, sizeof(ip));
				if (ret2 == ret_ok) {
					TRACE (ENTRIES, "Connecting to %s, ret=%d\n", ip, ret);
				}
			}
#endif

			if (ret == ret_ok) {
				sock->source_addr_current = addr;
				break;
			}

			addr = addr->ai_next;
			if (addr == NULL) {
				if (tested_all) {
					return ret_error;
				}

				tested_all = true;
				sock->source_addr_current = NULL;
				addr = addr_info;
				continue;
			}

			cherokee_socket_close(sock);
		}

		/* Update the new socket with the address info
		 */
		switch (sock->source_addr_current->ai_family) {
		case AF_INET:
			SOCKET_ADDR_IPv4(sock)->sin_port = htons(src->port);
			break;
		case AF_INET6:
			SOCKET_ADDR_IPv6(sock)->sin6_port = htons(src->port);
			break;
		default:
			SHOULDNT_HAPPEN;
			return ret_error;
		}

		ret = cherokee_socket_update_from_addrinfo (sock, sock->source_addr_current);
		if (unlikely (ret != ret_ok)) {
			return ret_error;
		}
	}

	/* Set non-blocking */
	ret = cherokee_fd_set_nonblocking (sock->socket, true);
	if (unlikely (ret != ret_ok)) {
		LOG_ERRNO (errno, cherokee_err_error, CHEROKEE_ERROR_SOURCE_NONBLOCK, sock->socket);
	}

	/* Set close-on-exec and reuse-address */
	cherokee_fd_set_closexec  (sock->socket);
	cherokee_fd_set_reuseaddr (sock->socket);

	/* On the off-chance a connect completes early, handle a deny here
	 * in addition to at the top of this routine.
	 */
	ret = cherokee_socket_connect (sock);
	if (ret == ret_deny && sock->source_addr_current && sock->source_addr_current->ai_next != NULL) {
		sock->source_addr_current = sock->source_addr_current->ai_next;
		cherokee_socket_close(sock);
		goto long_path;
	}

	return ret;
}


ret_t
cherokee_source_connect_polling (cherokee_source_t     *src,
                                 cherokee_socket_t     *socket,
                                 cherokee_connection_t *conn)
{
	ret_t ret;

	ret = cherokee_source_connect (src, socket);
	switch (ret) {
	case ret_ok:
		TRACE (ENTRIES, "Connected successfully fd=%d\n", socket->socket);
		return ret_ok;
	case ret_deny:
		break;
	case ret_eagain:
		ret = cherokee_thread_deactive_to_polling (CONN_THREAD(conn),
		                                           conn,
		                                           SOCKET_FD(socket),
		                                           FDPOLL_MODE_WRITE,
		                                           false);
		if (ret != ret_ok) {
			return ret_deny;
		}
		return ret_eagain;
	case ret_error:
		return ret_error;
	default:
		break;
	}

	TRACE (ENTRIES, "Couldn't connect%s", "\n");
	return ret_error;
}


static ret_t
set_host (cherokee_source_t *src, cherokee_buffer_t *host)
{
	ret_t                    ret;
	cherokee_resolv_cache_t *resolv;

	if (cherokee_buffer_is_empty (host))
		return ret_error;

	TRACE (ENTRIES, "Configuring source, setting host '%s'\n", host->buf);

	/* Original
	 */
	cherokee_buffer_add_buffer (&src->original, host);

	/* Unix socket
	 */
	if (host->buf[0] == '/' || host->buf[0] == '@') {
		cherokee_buffer_add_buffer (&src->unix_socket, host);
		return ret_ok;
	}

	/* Host name
	 */
	ret = cherokee_parse_host (host, &src->host, (cuint_t *)&src->port);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	/* Resolve and cache it
	 */
	ret = cherokee_resolv_cache_get_default (&resolv);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	cherokee_resolv_cache_get_ipstr (resolv, &src->host, NULL);
	return ret_ok;
}


ret_t
cherokee_source_configure (cherokee_source_t *src, cherokee_config_node_t *conf)
{
	cherokee_list_t        *i;
	cherokee_config_node_t *child;

	cherokee_config_node_foreach (i, conf) {
		child = CONFIG_NODE(i);

		if (equal_buf_str (&child->key, "host")) {
			if (set_host(src, &child->val) != ret_ok) return ret_error;
		} else if (equal_buf_str(&child->key, "keepalive")) {
			cherokee_boolean_t enabled;
			if ((!equal_buf_str(&child->val, "0") && !equal_buf_str(&child->val, "1")) || cherokee_atob(child->val.buf, &enabled) != ret_ok) return ret_error;
			src->keepalive = enabled;
		} else {
			cuint_t *target = NULL;
			cuint_t maximum = 86400;
			unsigned long value;
			if (equal_buf_str(&child->key, "keepalive_max")) { target = &src->keepalive_max; maximum = 1024; }
			else if (equal_buf_str(&child->key, "keepalive_timeout")) target = &src->keepalive_timeout;
			else if (equal_buf_str(&child->key, "keepalive_requests")) { target = &src->keepalive_requests; maximum = 65535; }
			else if (equal_buf_str(&child->key, "keepalive_age")) target = &src->keepalive_age;
			else if (equal_buf_str(&child->key, "connect_timeout")) target = &src->connect_timeout;
			else if (equal_buf_str(&child->key, "io_timeout")) target = &src->io_timeout;
			else if (equal_buf_str(&child->key, "retries")) { target = &src->retries; maximum = 3; }
			if (target) {
				const char *p = child->val.buf;
				if (!p || !*p) return ret_error;
				for (; *p; p++) if (*p < '0' || *p > '9') return ret_error;
				errno = 0; value = strtoul(child->val.buf, NULL, 10);
				if (errno || value > maximum ||
				    (!value && target != &src->retries && target != &src->connect_timeout && target != &src->io_timeout)) return ret_error;
				*target = value;
			}
		}

		/* Base class: do not display error here
		 */
	}

	return ret_ok;
}


ret_t
cherokee_source_copy_name (cherokee_source_t *src,
                           cherokee_buffer_t *buf)
{
	if (! cherokee_buffer_is_empty (&src->unix_socket)) {
		cherokee_buffer_add_buffer (buf, &src->unix_socket);
		return ret_ok;
	}

	cherokee_buffer_add_buffer  (buf, &src->host);
	cherokee_buffer_add_char    (buf, ':');
	cherokee_buffer_add_ulong10 (buf, src->port);

	return ret_ok;
}

