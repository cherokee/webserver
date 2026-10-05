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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_SOURCE_H
#define CHEROKEE_SOURCE_H

#include <cherokee/list.h>
#include <cherokee/buffer.h>
#include <cherokee/socket.h>
#include <cherokee/config_node.h>
#include <cherokee/connection.h>

CHEROKEE_BEGIN_DECLS

typedef enum {
	source_host,
	source_interpreter
} cherokee_source_type_t;

typedef struct {
	cherokee_source_type_t type;
	cherokee_buffer_t      original;
	cherokee_buffer_t      unix_socket;
	cherokee_buffer_t      host;
	cint_t                 port;

	/* -1 preserves each protocol's legacy default. */
	cint_t                keepalive;
	cuint_t               keepalive_max;
	cuint_t               keepalive_timeout;
	cuint_t               keepalive_requests;
	cuint_t               keepalive_age;
	cuint_t               connect_timeout;
	cuint_t               io_timeout;
	cuint_t               retries;
	void                 *reuse_pool;

	cherokee_func_free_t   free;
} cherokee_source_t;

#define SOURCE(s)  ((cherokee_source_t *)(s))

ret_t cherokee_source_new       (cherokee_source_t **src);
ret_t cherokee_source_free      (cherokee_source_t  *src);
ret_t cherokee_source_init      (cherokee_source_t  *src);
ret_t cherokee_source_mrproper  (cherokee_source_t  *src);

ret_t cherokee_source_configure (cherokee_source_t *src, cherokee_config_node_t *conf);
ret_t cherokee_source_connect   (cherokee_source_t *src, cherokee_socket_t *socket);

ret_t cherokee_source_connect_polling (cherokee_source_t     *src,
                                       cherokee_socket_t     *socket,
                                       cherokee_connection_t *conn);

ret_t cherokee_source_copy_name       (cherokee_source_t     *src,
                                       cherokee_buffer_t     *buf);

typedef enum {
	source_protocol_fcgi,
	source_protocol_http
} cherokee_source_protocol_t;

/* Sockets are transferred exclusively; protocol parsers decide eligibility. */
ret_t cherokee_source_reuse_get (cherokee_source_t *, cherokee_source_protocol_t,
                                 cherokee_socket_t *, uint64_t *, cuint_t *, cushort_t *);
ret_t cherokee_source_reuse_put (cherokee_source_t *, cherokee_source_protocol_t,
                                 cherokee_socket_t *, uint64_t, cuint_t, cushort_t, cuint_t);
ret_t cherokee_source_reuse_cleanup (cherokee_buffer_t *, void *, void *);
cherokee_boolean_t cherokee_source_socket_clean (cherokee_socket_t *);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_SOURCE_H */
