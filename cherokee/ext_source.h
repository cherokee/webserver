/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_EXT_SOURCE_H
#define CHEROKEE_EXT_SOURCE_H

#include "list.h"
#include "buffer.h"
#include "typed_table.h"
#include "socket.h"


CHEROKEE_BEGIN_DECLS

typedef struct {
	struct list_head           entry;

	cherokee_buffer_t          interpreter;

	cherokee_buffer_t          original_server;
	cherokee_buffer_t          host;
	cint_t                     port;
	cherokee_buffer_t          unix_socket;
	
	char                     **custom_env;
	cuint_t                    custom_env_len;

	cherokee_typed_free_func_t free_func;
} cherokee_ext_source_t;


typedef struct {
	cherokee_ext_source_t  entry;

	/* Current server
	 */
	cherokee_ext_source_t  *current_server;
#ifdef HAVE_PTHREAD
	pthread_mutex_t         current_server_lock;
#endif	
} cherokee_ext_source_head_t;

 
#define EXT_SOURCE(x)       ((cherokee_ext_source_t *)(x))
#define EXT_SOURCE_HEAD(x)  ((cherokee_ext_source_head_t *)(x))


/* External source
 */
ret_t cherokee_ext_source_new       (cherokee_ext_source_t **server);
ret_t cherokee_ext_source_free      (cherokee_ext_source_t  *server);

ret_t cherokee_ext_source_add_env   (cherokee_ext_source_t  *server, char *env, char *val);
ret_t cherokee_ext_source_connect   (cherokee_ext_source_t  *server, cherokee_socket_t *socket);
ret_t cherokee_ext_source_spawn_srv (cherokee_ext_source_t  *server);

/* External source head
 */
ret_t cherokee_ext_source_head_new  (cherokee_ext_source_head_t **serverf);
ret_t cherokee_ext_source_get_next  (cherokee_ext_source_head_t  *serverf, list_t *server_list, cherokee_ext_source_t **next);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_EXT_SOURCE_H */
