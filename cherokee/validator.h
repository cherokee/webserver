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

#ifndef CHEROKEE_VALIDATOR_H
#define CHEROKEE_VALIDATOR_H

#include "common.h"
#include "buffer.h"
#include "module.h"
#include "table.h"
#include "http.h"
#include "connection.h"


/* Callback function definitions
 */
typedef ret_t (* validator_func_new_t)         (void **validator, cherokee_table_t *properties); 
typedef ret_t (* validator_func_check_t)       (void  *validator, void *conn);
typedef ret_t (* validator_func_add_headers_t) (void  *validator, void *conn, cherokee_buffer_t *buf);


typedef struct {
	/* Base
	 */
	cherokee_module_t module;
	
	/* Pure virtual methods	
	 */
	validator_func_check_t       check;
	validator_func_add_headers_t add_headers;

	/* Properties
	 */
	cherokee_http_auth_t support;

	/* Authentication info
	 */
	cherokee_buffer_t user;
	cherokee_buffer_t passwd;
	cherokee_buffer_t realm;
	cherokee_buffer_t response;
	cherokee_buffer_t uri;
	cherokee_buffer_t qop;
	cherokee_buffer_t nonce;
	cherokee_buffer_t cnonce;
	cherokee_buffer_t algorithm;
	cherokee_buffer_t nc;
	
} cherokee_validator_t;

#define VALIDATOR(x) ((cherokee_validator_t *)(x))


ret_t cherokee_validator_init_base (cherokee_validator_t *validator);
ret_t cherokee_validator_free_base (cherokee_validator_t *validator);   

ret_t cherokee_validator_free         (cherokee_validator_t *validator);
ret_t cherokee_validator_check        (cherokee_validator_t *validator, void *conn);
ret_t cherokee_validator_add_headers  (cherokee_validator_t *validator, void *conn, cherokee_buffer_t *buf);

ret_t cherokee_validator_parse_basic  (cherokee_validator_t *validator, char *str, cuint_t str_len);
ret_t cherokee_validator_parse_digest (cherokee_validator_t *validator, char *str, cuint_t str_len);

ret_t cherokee_validator_digest_response (cherokee_validator_t *validator, char *A1, cherokee_buffer_t *buf, cherokee_connection_t *conn);


#endif /* CHEROKEE_VALIDATOR_H */
