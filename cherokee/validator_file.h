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

#ifndef CHEROKEE_VALIDATOR_FILE_H
#define CHEROKEE_VALIDATOR_FILE_H

#include "validator.h"
#include "connection.h"

typedef enum {
	val_path_full,
	val_path_local_dir
} cherokee_validator_path_t;

typedef struct {
	cherokee_module_props_t   base;
	cherokee_buffer_t         password_file;
	cherokee_validator_path_t password_path_type;
} cherokee_validator_file_props_t;

typedef struct {
	cherokee_validator_t      validator;
} cherokee_validator_file_t;

#define VFILE(x)          ((cherokee_validator_file_t *)(x))
#define PROP_VFILE(p)     ((cherokee_validator_file_props_t *)(p))
#define VAL_VFILE_PROP(x) (PROP_VFILE (MODULE(x)->props))


/* Validator
 */

ret_t cherokee_validator_file_init_base (cherokee_validator_file_t        *validator,
					 cherokee_validator_file_props_t  *props,
					 cherokee_plugin_info_validator_t *info);

ret_t cherokee_validator_file_free_base (cherokee_validator_file_t        *validator);

ret_t cherokee_validator_file_configure (cherokee_config_node_t            *conf,
					 cherokee_server_t                 *srv,
					 cherokee_module_props_t          **_props);

/* Validator properties
 */

ret_t cherokee_validator_file_props_init_base  (cherokee_validator_file_props_t *props,
						module_func_props_free_t         free_func);

ret_t cherokee_validator_file_props_free_base  (cherokee_validator_file_props_t *props);


/* Utilities
 */

ret_t cherokee_validator_file_get_full_path    (cherokee_validator_file_t  *validator,
						cherokee_connection_t      *conn,
						cherokee_buffer_t         **ret_buf,
						cherokee_buffer_t          *tmp);

#endif /* CHEROKEE_VALIDATOR_FILE_H */
