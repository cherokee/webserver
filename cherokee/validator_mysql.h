/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Brian Rosner (brosner@gmail.com)
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

#ifndef CHEROKEE_VALIDATOR_MYSQL_H
#define CHEROKEE_VALIDATOR_MYSQL_H

#include "validator.h"
#include "connection.h"

#include <mysql.h>

typedef struct {
	cherokee_validator_t	validator;
	MYSQL		       *conn;
} cherokee_validator_mysql_t;

typedef struct {
	cherokee_module_props_t	base;
	
	cherokee_buffer_t	host;
	cint_t			port;
	cherokee_buffer_t       unix_socket;
	
	cherokee_buffer_t       user;
	cherokee_buffer_t	passwd;
	cherokee_buffer_t	database;
	cherokee_buffer_t	query;

	cherokee_boolean_t      use_md5_passwd;
	
} cherokee_validator_mysql_props_t;

#define MYSQL(x)           ((cherokee_validator_mysql_t *)(x))
#define PROP_MYSQL(p)      ((cherokee_validator_mysql_props_t *)(p))
#define VAL_MYSQL_PROP(x)  (PROP_MYSQL (MODULE(x)->props))

ret_t cherokee_validator_mysql_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props);

ret_t cherokee_validator_mysql_new         (cherokee_validator_mysql_t **mysql, cherokee_module_props_t *props);
ret_t cherokee_validator_mysql_free        (cherokee_validator_mysql_t  *mysql);
ret_t cherokee_validator_mysql_check       (cherokee_validator_mysql_t  *mysql, cherokee_connection_t *conn);
ret_t cherokee_validator_mysql_add_headers (cherokee_validator_mysql_t  *mysql, cherokee_connection_t *conn, cherokee_buffer_t *buf);

#endif /* CHEROKEE_VALIDATOR_MYSQL_H */
