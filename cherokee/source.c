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

#define ENTRIES "source,src"


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
	src->addr_current = NULL;

	return ret_ok;
}


ret_t
cherokee_source_mrproper (cherokee_source_t *src)
{
	if (src->free) {
		src->free (src);
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
		if (ret != ret_deny || src->addr_current->ai_next == NULL)
			return ret;
		/* Fall through and attempt to try another address... */
		cherokee_socket_close(sock);
		src->addr_current = src->addr_current->ai_next;
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
		if (src->addr_current) {
			tested_all = false;
			addr = src->addr_current;
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
				src->addr_current = addr;
				break;
			}

			addr = addr->ai_next;
			if (addr == NULL) {
				if (tested_all) {
					return ret_error;
				}

				tested_all = true;
				src->addr_current = NULL;
				addr = addr_info;
				continue;
			}

			cherokee_socket_close(sock);
		}

		/* Update the new socket with the address info
		 */
		switch (src->addr_current->ai_family) {
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

		ret = cherokee_socket_update_from_addrinfo (sock, src->addr_current, 0);
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
	if (ret == ret_deny && src->addr_current && src->addr_current->ai_next != NULL) {
		src->addr_current = src->addr_current->ai_next;
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
			set_host (src, &child->val);
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

