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

#ifndef CHEROKEE_SOURCE_INTERPRETER_H
#define CHEROKEE_SOURCE_INTERPRETER_H

#include "source.h"


CHEROKEE_BEGIN_DECLS


typedef struct {
	cherokee_source_t   source;

	cherokee_buffer_t   interpreter;
	char              **custom_env;
	cuint_t             custom_env_len;
	cherokee_boolean_t  debug;

	CHEROKEE_MUTEX_T   (launching_mutex);
	cherokee_boolean_t  launching;
} cherokee_source_interpreter_t;

#define SOURCE_INT(s)  ((cherokee_source_interpreter_t *)(s))


ret_t cherokee_source_interpreter_new       (cherokee_source_interpreter_t **src);
ret_t cherokee_source_interpreter_configure (cherokee_source_interpreter_t  *src, cherokee_config_node_t *conf);

ret_t cherokee_source_interpreter_add_env   (cherokee_source_interpreter_t *src, char *env, char *val);
ret_t cherokee_source_interpreter_spawn     (cherokee_source_interpreter_t *src);

ret_t cherokee_source_interpreter_connect_polling (cherokee_source_interpreter_t *src, 
						   cherokee_socket_t             *socket,
						   cherokee_connection_t         *conn,
						   time_t                        *spawned);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_SOURCE_INTERPRETER_H */
