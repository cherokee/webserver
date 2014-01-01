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
#include "bind.h"
#include "util.h"

ret_t
cherokee_bind_new (cherokee_bind_t **listener)
{
	CHEROKEE_NEW_STRUCT(n,bind);

	INIT_LIST_HEAD (&n->listed);

	cherokee_buffer_init (&n->ip);
	cherokee_socket_init (&n->socket);
	n->port   = 0;
	n->id     = 0;
	n->family = 0;

	cherokee_buffer_init (&n->server_string);
	cherokee_buffer_init (&n->server_string_w_port);
	cherokee_buffer_init (&n->server_address);
	cherokee_buffer_init (&n->server_port);

	n->accept_continuous     = 0;
	n->accept_continuous_max = 0;
	n->accept_recalculate    = 0;

	*listener = n;
	return ret_ok;
}


ret_t
cherokee_bind_free (cherokee_bind_t *listener)
{
	cherokee_socket_close (&listener->socket);
	cherokee_socket_mrproper (&listener->socket);

	cherokee_buffer_mrproper (&listener->ip);

	cherokee_buffer_mrproper (&listener->server_string);
	cherokee_buffer_mrproper (&listener->server_string_w_port);
	cherokee_buffer_mrproper (&listener->server_address);
	cherokee_buffer_mrproper (&listener->server_port);

	free (listener);
	return ret_ok;
}


static ret_t
build_strings (cherokee_bind_t         *listener,
	       cherokee_server_token_t  token)
{
	ret_t ret;
	char  server_ip[CHE_INET_ADDRSTRLEN+1];

	/* server_string_
	 */
	cherokee_buffer_clean (&listener->server_string_w_port);
	ret = cherokee_version_add_w_port (&listener->server_string_w_port,
	                                   token, listener->port);
	if (ret != ret_ok)
		return ret;

	cherokee_buffer_clean (&listener->server_string);
	ret = cherokee_version_add_simple (&listener->server_string, token);
	if (ret != ret_ok)
		return ret;

	/* server_address
	 */
	cherokee_socket_ntop (&listener->socket, server_ip, sizeof(server_ip)-1);
	cherokee_buffer_add (&listener->server_address, server_ip, strlen(server_ip));

	/* server_port
	 */
	cherokee_buffer_add_va (&listener->server_port, "%d", listener->port);
	return ret_ok;
}


ret_t
cherokee_bind_configure (cherokee_bind_t        *listener,
                         cherokee_config_node_t *conf)
{
	ret_t              ret;
	cherokee_boolean_t tls;
	cherokee_buffer_t *buf;

	ret = cherokee_atoi (conf->key.buf, &listener->id);
	if (unlikely (ret != ret_ok))
		return ret_error;

	ret = cherokee_config_node_read (conf, "interface", &buf);
	if (ret == ret_ok) {
		cherokee_buffer_mrproper (&listener->ip);
		cherokee_buffer_add_buffer (&listener->ip, buf);
		if (cherokee_string_is_ipv6 (&listener->ip)) {
			listener->family = AF_INET6;
		} else {
			listener->family = AF_INET;
		}
	}

	ret = cherokee_config_node_read_uint (conf, "port", &listener->port);
	if (ret != ret_ok) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_BIND_PORT_NEEDED);
		return ret_error;
	}

	ret = cherokee_config_node_read_bool (conf, "tls", &tls);
	if (ret == ret_ok) {
		listener->socket.is_tls = (tls) ? TLS : non_TLS;
	}

	return ret_ok;
}


ret_t
cherokee_bind_set_default (cherokee_bind_t *listener)
{
	listener->port = 80;
	return ret_ok;
}


static ret_t
set_socket_opts (int socket)
{
	ret_t                    ret;
#ifdef SO_ACCEPTFILTER
	struct accept_filter_arg afa;
#endif

	/* Set 'close-on-exec'
	 */
	ret = cherokee_fd_set_closexec (socket);
	if (ret != ret_ok)
		return ret;

	/* To re-bind without wait to TIME_WAIT. It prevents 2MSL
	 * delay on accept.
	 */
	ret = cherokee_fd_set_reuseaddr (socket);
	if (ret != ret_ok)
		return ret;

	/* Set no-delay mode:
	 * If no clients are waiting, accept() will return -1 immediately
	 */
	ret = cherokee_fd_set_nodelay (socket, true);
	if (ret != ret_ok)
		return ret;

	/* TCP_MAXSEG:
	 * The maximum size of a TCP segment is based on the network MTU for des-
	 * tinations on local networks or on a default MTU of 576 bytes for desti-
	 * nations on nonlocal networks.  The default behavior can be altered by
	 * setting the TCP_MAXSEG option to an integer value from 1 to 65,535.
	 * However, TCP will not use a maximum segment size smaller than 32 or
	 * larger than the local network MTU.  Setting the TCP_MAXSEG option to a
	 * value of zero results in default behavior.  The TCP_MAXSEG option can
	 * only be set prior to calling listen or connect on the socket.  For pas-
	 * sive connections, the TCP_MAXSEG option value is inherited from the
	 * listening socket. This option takes an int value, with a range of 0 to
	 * 65535.
	 */
#ifdef TCP_MAXSEG
	on = 64000;
	setsockopt (socket, SOL_SOCKET, TCP_MAXSEG, &on, sizeof(on));

	/* Do no check the returned value */
#endif

	/* TCP_DEFER_ACCEPT:
	 * Allows a listener to be awakened only when data arrives on the socket.
	 * Takes an integer value (seconds), this can bound the maximum number of
	 * attempts TCP will make to complete the connection. This option should
	 * not be used in code intended to be portable.
	 *
	 * Give clients 5s to send first data packet
	 */
#ifdef TCP_DEFER_ACCEPT
	on = 5;
	setsockopt (socket, SOL_TCP, TCP_DEFER_ACCEPT, &on, sizeof(on));
#endif

	/* SO_ACCEPTFILTER:
	 * FreeBSD accept filter for HTTP:
	 *
	 * http://www.freebsd.org/cgi/man.cgi?query=accf_http
	 */
#ifdef SO_ACCEPTFILTER
	memset (&afa, 0, sizeof(afa));
	strcpy (afa.af_name, "httpready");

	setsockopt (socket, SOL_SOCKET, SO_ACCEPTFILTER, &afa, sizeof(afa));
#endif

	return ret_ok;
}


static ret_t
init_socket (cherokee_bind_t *listener)
{
	ret_t ret;

	/* Create the socket, and set its properties
	 */
	ret = cherokee_socket_create_fd (&listener->socket, listener->family);
	if ((ret != ret_ok) || (SOCKET_FD(&listener->socket) < 0)) {
		return ret_error;
	}

	/* Set the socket properties
	 */
	ret = set_socket_opts (SOCKET_FD(&listener->socket));
	if (ret != ret_ok) {
		goto error;
	}

	/* Bind the socket
	 */
	ret = cherokee_socket_bind (&listener->socket, listener->port, &listener->ip);
	if (ret != ret_ok) {
		goto error;
	}

	return ret_ok;

error:
	/* The socket was already opened by _set_client
	 */
	cherokee_socket_close (&listener->socket);
	return ret;
}


ret_t
cherokee_bind_init_port (cherokee_bind_t         *listener,
                         cuint_t                  listen_queue,
                         cherokee_boolean_t       ipv6,
                         cherokee_server_token_t  token)
{
	ret_t ret;
	int family = listener->family;

	/* Init the port
	 */
#ifdef HAVE_IPV6
	if (ipv6 && (family == 0 || family == AF_INET6)) {
		listener->family = AF_INET6;
		ret = init_socket (listener);
	} else
#endif
	{
		ret = ret_not_found;
	}

	if (ret != ret_ok && (family == 0 || family == AF_INET)) {
		listener->family = AF_INET;
		ret = init_socket (listener);
	}

	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_BIND_COULDNT_BIND_PORT,
		              listener->port, getuid(), getgid());
		goto error;
	}

	/* Listen
	 */
	ret = cherokee_socket_listen (&listener->socket, listen_queue);
	if (ret != ret_ok) {
		goto error;
	}

	/* Build the strings
	 */
	ret = build_strings (listener, token);
	if (ret != ret_ok) {
		goto error;
	}

	return ret_ok;

error:
	cherokee_socket_close (&listener->socket);
	return ret_error;
}


ret_t
cherokee_bind_accept_more (cherokee_bind_t *listener,
                           ret_t            prev_ret)
{
	/* Failed to accept
	 */
	if (prev_ret != ret_ok) {
		listener->accept_continuous = 0;

		if (listener->accept_recalculate)
			listener->accept_recalculate -= 1;
		else
			listener->accept_recalculate  = 10;

		return ret_deny;
	}

	/* Did accept a connection
	 */
	listener->accept_continuous++;

	if (listener->accept_recalculate <= 0) {
		listener->accept_continuous_max = listener->accept_continuous;
		return ret_ok;
	}

	if (listener->accept_continuous < listener->accept_continuous_max) {
		return ret_ok;
	}

	listener->accept_continuous = 0;
	return ret_deny;
}
