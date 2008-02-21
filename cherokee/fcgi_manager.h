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

#ifndef CHEROKEE_FCGI_MANAGER_H
#define CHEROKEE_FCGI_MANAGER_H

#include "common.h"
#include "connection.h"
#include "socket.h"
#include "thread.h"
#include "source.h"


typedef struct {
	cherokee_connection_t *conn;
	cherokee_boolean_t     eof;
} conn_entry_t;

typedef struct {
	cherokee_socket_t      socket;
	cherokee_buffer_t      read_buffer;
	cherokee_source_t     *source;
	void                  *dispatcher;

	cherokee_boolean_t    first_connect;	
	char                  generation;
	cuint_t               pipeline;
	cherokee_boolean_t    keepalive;

	struct {
		conn_entry_t *id2conn;
		cuint_t       size;
		cuint_t       len;
	} conn;
} cherokee_fcgi_manager_t;


#define FCGI_MANAGER(f) ((cherokee_fcgi_manager_t *)(f))

ret_t cherokee_fcgi_manager_init        (cherokee_fcgi_manager_t *mgr, void *dispatcher, cherokee_source_t *src, cherokee_boolean_t keepalive, cuint_t pipeline);
ret_t cherokee_fcgi_manager_mrproper    (cherokee_fcgi_manager_t *mgr);

ret_t cherokee_fcgi_manager_register    (cherokee_fcgi_manager_t *mgr, cherokee_connection_t *conn, cuint_t *id, cuchar_t *gen);
ret_t cherokee_fcgi_manager_unregister  (cherokee_fcgi_manager_t *mgr, cherokee_connection_t *conn);

ret_t cherokee_fcgi_manager_ensure_is_connected (cherokee_fcgi_manager_t *mgr, cherokee_thread_t *thd);
ret_t cherokee_fcgi_manager_send_remove         (cherokee_fcgi_manager_t *mgr, cherokee_buffer_t *buf);
ret_t cherokee_fcgi_manager_step                (cherokee_fcgi_manager_t *mgr);

/* Internal use 
 */
char  cherokee_fcgi_manager_supports_pipelining (cherokee_fcgi_manager_t *mgr);

#endif /* CHEROKEE_FCGI_MANAGER_H */
