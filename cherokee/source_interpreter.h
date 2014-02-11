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

#ifndef CHEROKEE_SOURCE_INTERPRETER_H
#define CHEROKEE_SOURCE_INTERPRETER_H

#include "source.h"
#include <unistd.h>

CHEROKEE_BEGIN_DECLS

typedef enum {
	spawn_unknown,
	spawn_services,
	spawn_local
} cherokee_source_interpreter_spawn_t;

typedef struct {
	cherokee_source_t    source;
	cherokee_buffer_t    interpreter;
	cherokee_buffer_t    chroot;

	cherokee_boolean_t   env_inherited;
	char               **custom_env;
	cuint_t              custom_env_len;

	cuint_t              timeout;
	cherokee_boolean_t   debug;
	pid_t                pid;
	time_t               last_connect;
	time_t               spawning_since;
	time_t               spawning_since_fails;

	cherokee_buffer_t    change_user_name;
	uid_t                change_user;
	gid_t                change_group;

	CHEROKEE_MUTEX_T                    (launching_mutex);
	cherokee_boolean_t                   launching;
	cherokee_source_interpreter_spawn_t  spawn_type;
} cherokee_source_interpreter_t;

#define SOURCE_INT(s)  ((cherokee_source_interpreter_t *)(s))


ret_t cherokee_source_interpreter_new       (cherokee_source_interpreter_t **src);

ret_t cherokee_source_interpreter_configure (cherokee_source_interpreter_t  *src,
                                             cherokee_config_node_t         *conf,
                                             int                             prio);

ret_t cherokee_source_interpreter_spawn     (cherokee_source_interpreter_t  *src,
                                             cherokee_logger_writer_t       *error_writer);

ret_t cherokee_source_interpreter_connect_polling (cherokee_source_interpreter_t *src,
                                                   cherokee_socket_t             *socket,
                                                   cherokee_connection_t         *conn);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_SOURCE_INTERPRETER_H */
