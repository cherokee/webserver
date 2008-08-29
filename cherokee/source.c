/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
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

	src->type = source_host;
	src->port = -1;
	src->free = NULL;

	return ret_ok;
}


ret_t 
cherokee_source_mrproper (cherokee_source_t *src)
{
	if (src->free)
		src->free (src);

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
	if (sock->socket >= 0)
		goto out;

	/* Get required objects
	 */
	ret = cherokee_resolv_cache_get_default (&resolv);
        if (unlikely (ret!=ret_ok)) 
		return ret;

	/* UNIX socket
	 */
	if (! cherokee_buffer_is_empty (&src->unix_socket)) {
		ret = cherokee_socket_set_client (sock, AF_UNIX);
		if (ret != ret_ok) 
			return ret;

		/* Copy the unix socket path */
		ret = cherokee_socket_gethostbyname (sock, &src->unix_socket);
		if (ret != ret_ok)
			return ret;

		/* Set non-blocking */
		ret = cherokee_fd_set_nonblocking (sock->socket, true);
		if (ret != ret_ok) {
			PRINT_ERRNO (errno, "Failed to set nonblocking (fd=%d): ${errno}\n",
				     sock->socket);
		}

		goto out;
	}

	/* INET socket
	 */
	ret = cherokee_socket_set_client (sock, AF_INET);
	if (ret != ret_ok) return ret;
	
	/* Query the host */
	ret = cherokee_resolv_cache_get_host (resolv, src->host.buf, sock);
	if (ret != ret_ok) return ret;
	
	SOCKET_ADDR_IPv4(sock)->sin_port = htons(src->port);

	/* Set non-blocking */
	ret = cherokee_fd_set_nonblocking (sock->socket, true);
	if (ret != ret_ok) {
		PRINT_ERRNO (errno, "Failed to set nonblocking (fd=%d): ${errno}\n",
			     sock->socket);
	}

out: 	
	return cherokee_socket_connect (sock);
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
	char *p;

	if (cherokee_buffer_is_empty (host))
		return ret_error;
	
	TRACE (ENTRIES, "Configuring source, setting host '%s'\n", host->buf);

	/* Original
	 */
	cherokee_buffer_add_buffer (&src->original, host);
	
	/* Unix socket
	 */
	if (host->buf[0] == '/') {
		cherokee_buffer_add_buffer (&src->unix_socket, host);
		return ret_ok;
	} 
	
	/* Host name
	 */
	p = strchr (host->buf, ':');
	if (p == NULL) {
		cherokee_buffer_add_buffer (&src->host, host);
		return ret_ok;
	} 
	
	/* Host name + port
	 */
	*p = '\0';
	src->port = atoi (p+1);
	cherokee_buffer_add (&src->host, host->buf, p - host->buf);
	*p = ':';
	
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
