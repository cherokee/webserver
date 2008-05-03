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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_VALIDATOR_H
#define CHEROKEE_VALIDATOR_H

#include "common.h"
#include "buffer.h"
#include "module.h"
#include "http.h"
#include "connection.h"
#include "config_node.h"


CHEROKEE_BEGIN_DECLS

/* Callback function definitions
 */
typedef ret_t (* validator_func_new_t)         (void **validator, cherokee_module_props_t *props); 
typedef ret_t (* validator_func_check_t)       (void  *validator, void *conn);
typedef ret_t (* validator_func_add_headers_t) (void  *validator, void *conn, cherokee_buffer_t *buf);
typedef ret_t (* validator_func_configure_t)   (cherokee_config_node_t *, cherokee_server_t *, cherokee_module_props_t **);


/* Data types
 */
typedef struct {
	cherokee_module_t            module;
	
	/* Pure virtual methods	
	 */
	validator_func_check_t       check;
	validator_func_add_headers_t add_headers;

	/* Properties
	 */
	cherokee_http_auth_t         support;

	/* Authentication info
	 */
	cherokee_buffer_t            user;
	cherokee_buffer_t            passwd;
	cherokee_buffer_t            realm;
	cherokee_buffer_t            response;
	cherokee_buffer_t            uri;
	cherokee_buffer_t            qop;
	cherokee_buffer_t            nonce;
	cherokee_buffer_t            cnonce;
	cherokee_buffer_t            algorithm;
	cherokee_buffer_t            nc;
	
} cherokee_validator_t;

#define VALIDATOR(x)             ((cherokee_validator_t *)(x))


/* Properties data type
 */
typedef struct {
	cherokee_module_props_t base;
	cherokee_http_auth_t    valid_methods;
} cherokee_validator_props_t;

#define VALIDATOR_PROPS(x)                 ((cherokee_validator_props_t *)(x))


/* Easy initialization
 */
#define VALIDATOR_CONF_PROTOTYPE(name)                              \
	ret_t cherokee_validator_ ## name ## _configure (           \
		cherokee_config_node_t   *,                         \
		cherokee_server_t        *,                         \
	 	cherokee_module_props_t **)

#define PLUGIN_INFO_VALIDATOR_EASY_INIT(name,methods)               \
	VALIDATOR_CONF_PROTOTYPE(name);                             \
                                                                    \
	PLUGIN_INFO_VALIDATOR_INIT(name, cherokee_validator,        \
		(void *)cherokee_validator_ ## name ## _new,        \
		(void *)cherokee_validator_ ## name ## _configure,  \
                methods)     

#define PLUGIN_INFO_VALIDATOR_EASIEST_INIT(name,methods)            \
	PLUGIN_EMPTY_INIT_FUNCTION(name)                            \
	PLUGIN_INFO_VALIDATOR_EASY_INIT(name,methods)


/* Validator methods
 */
ret_t cherokee_validator_init_base       (cherokee_validator_t *validator, cherokee_validator_props_t *props, cherokee_plugin_info_validator_t *info);
ret_t cherokee_validator_free_base       (cherokee_validator_t *validator);   

/* Validator virtual methods
 */
ret_t cherokee_validator_configure       (cherokee_config_node_t *conf, void *config_entry);
ret_t cherokee_validator_free            (cherokee_validator_t *validator);
ret_t cherokee_validator_check           (cherokee_validator_t *validator, void *conn);
ret_t cherokee_validator_add_headers     (cherokee_validator_t *validator, void *conn, cherokee_buffer_t *buf);

/* Validator internal methods
 */
ret_t cherokee_validator_parse_basic     (cherokee_validator_t *validator, char *str, cuint_t str_len);
ret_t cherokee_validator_parse_digest    (cherokee_validator_t *validator, char *str, cuint_t str_len);
ret_t cherokee_validator_digest_response (cherokee_validator_t *validator, char *A1, cherokee_buffer_t *buf, cherokee_connection_t *conn);
ret_t cherokee_validator_digest_check    (cherokee_validator_t *validator, cherokee_buffer_t *passwd, cherokee_connection_t *conn);

/* Validator properties methods
 */
ret_t cherokee_validator_props_init_base  (cherokee_validator_props_t *props, module_func_props_free_t free_func);
ret_t cherokee_validator_props_free_base  (cherokee_validator_props_t *props);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_VALIDATOR_H */
