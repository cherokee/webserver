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

#ifndef CHEROKEE_HANDLER_CGI_BASE_H
#define CHEROKEE_HANDLER_CGI_BASE_H

#include "common-internal.h"

#include "buffer.h"
#include "handler.h"
#include "list.h"
#include "connection.h"


typedef struct cherokee_handler_cgi_base cherokee_handler_cgi_base_t;

typedef void  (* cherokee_handler_cgi_base_add_env_pair_t)  (cherokee_handler_cgi_base_t *cgi,
							     char *name,    int name_len,
							     char *content, int content_len);

typedef ret_t (* cherokee_handler_cgi_base_read_from_cgi_t) (cherokee_handler_cgi_base_t *cgi, 
							     cherokee_buffer_t *buffer);

typedef enum {
	hcgi_phase_build_headers,
	hcgi_phase_send_headers,
	hcgi_phase_send_post
} cherokee_handler_cgi_base_phase_t;


struct cherokee_handler_cgi_base {
	cherokee_handler_t                handler;
	cherokee_handler_cgi_base_phase_t init_phase;
	
	char               *script_alias;
	char               *extra_param;
	list_t             *system_env;
	size_t              content_length;
	cherokee_boolean_t  is_error_handler;
	cherokee_boolean_t  check_file;

	cuint_t             change_user;
	cuint_t             got_eof;

	cherokee_buffer_t   executable;
	cherokee_buffer_t   param; 
	cherokee_buffer_t   param_extra;

	cherokee_buffer_t   data; 

	/* Virtual methods
	 */
	cherokee_handler_cgi_base_add_env_pair_t  add_env_pair;
	cherokee_handler_cgi_base_read_from_cgi_t read_from_cgi;
} ;

#define CGI_BASE(c)  ((cherokee_handler_cgi_base_t *)(c))


ret_t cherokee_handler_cgi_base_init           (cherokee_handler_cgi_base_t              *hdl, 
						cherokee_connection_t                    *conn,
						cherokee_table_t                         *properties, 
						cherokee_handler_cgi_base_add_env_pair_t  add_env_pair,
						cherokee_handler_cgi_base_read_from_cgi_t read_from_cgi);

ret_t cherokee_handler_cgi_base_free           (cherokee_handler_cgi_base_t *hdl);

void  cherokee_handler_cgi_base_add_parameter  (cherokee_handler_cgi_base_t *cgi, char *name, cuint_t len);
ret_t cherokee_handler_cgi_base_extract_path   (cherokee_handler_cgi_base_t *cgi, cherokee_boolean_t check_filename);

ret_t cherokee_handler_cgi_base_add_headers    (cherokee_handler_cgi_base_t *cgi, cherokee_buffer_t *buffer);
ret_t cherokee_handler_cgi_base_step           (cherokee_handler_cgi_base_t *cgi, cherokee_buffer_t *buffer);

ret_t cherokee_handler_cgi_base_split_pathinfo (cherokee_handler_cgi_base_t *cgi, 
						cherokee_buffer_t           *buf, 
						int                          pos,
						int                          allow_dirs);

ret_t cherokee_handler_cgi_base_build_envp      (cherokee_handler_cgi_base_t *cgi, cherokee_connection_t *conn);
ret_t cherokee_handler_cgi_base_build_basic_env (cherokee_handler_cgi_base_t              *cgi, 
						 cherokee_handler_cgi_base_add_env_pair_t  set_env_pair,
						 cherokee_connection_t                    *conn,
						 cherokee_buffer_t                        *tmp);

#endif /* CHEROKEE_HANDLER_CGI_BASE_H */

