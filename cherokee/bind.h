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

#ifndef CHEROKEE_BIND_H
#define CHEROKEE_BIND_H

#include "common-internal.h"
#include "list.h"
#include "buffer.h"
#include "config_node.h"
#include "socket.h"
#include "version.h"

typedef struct {
	cherokee_list_t    listed;
	int                id;

	/* Listener */
	cherokee_socket_t  socket;

	/* Information */
	cherokee_buffer_t  ip;
	cuint_t            port;
	cuint_t            family;

	/* Strings */
	cherokee_buffer_t  server_string;
	cherokee_buffer_t  server_string_w_port;

	cherokee_buffer_t  server_address;
	cherokee_buffer_t  server_port;

	/* Accepting */
	cuint_t            accept_continuous;
	cuint_t            accept_continuous_max;
	cuint_t            accept_recalculate;
} cherokee_bind_t;

#define BIND(b)        ((cherokee_bind_t *)(b))
#define BIND_IS_TLS(b) (BIND(b)->socket.is_tls == TLS)


ret_t cherokee_bind_new  (cherokee_bind_t **listener);
ret_t cherokee_bind_free (cherokee_bind_t  *listener);

ret_t cherokee_bind_configure   (cherokee_bind_t *listener, cherokee_config_node_t *config);
ret_t cherokee_bind_set_default (cherokee_bind_t *listener);
ret_t cherokee_bind_accept_more (cherokee_bind_t *listener, ret_t prev_ret);

ret_t cherokee_bind_init_port   (cherokee_bind_t         *listener,
				 cuint_t                  listen_queue,
				 cherokee_boolean_t       ipv6,
				 cherokee_server_token_t  token);

#endif /* CHEROKEE_BIND_H */
