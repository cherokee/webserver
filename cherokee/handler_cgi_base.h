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

#ifndef CHEROKEE_HANDLER_CGI_BASE_H
#define CHEROKEE_HANDLER_CGI_BASE_H

#include "common-internal.h"

#include "buffer.h"
#include "handler.h"
#include "list.h"
#include "connection.h"

#define SUPPORT_XSENDFILE

/* Plug-in initialization
 */
#ifndef SUPPORT_XSENDFILE
# define CGI_LIB_INIT(name, methods) \
	PLUGIN_INFO_HANDLER_EASY_INIT (name, (methods))
#else
# define CGI_LIB_INIT(name, methods)                          \
	PLUGIN_INIT_PROTOTYPE(name) {	   	              \
		PLUGIN_INIT_ONCE_CHECK(name);                 \
		cherokee_plugin_loader_load (loader, "file"); \
	}                                                     \
        PLUGIN_INFO_HANDLER_EASY_INIT (name, (methods))
#endif

/* Function types
 */
typedef struct cherokee_handler_cgi_base cherokee_handler_cgi_base_t;

typedef void  (* cherokee_handler_cgi_base_add_env_pair_t)  (cherokee_handler_cgi_base_t *cgi,
							     char *name,    int name_len,
							     char *content, int content_len);

typedef ret_t (* cherokee_handler_cgi_base_read_from_cgi_t) (cherokee_handler_cgi_base_t *cgi, 
							     cherokee_buffer_t *buffer);

typedef enum {
	hcgi_phase_build_headers,
	hcgi_phase_connect,
	hcgi_phase_send_headers,
	hcgi_phase_send_post
} cherokee_handler_cgi_base_phase_t;

/* Data structure
 */
struct cherokee_handler_cgi_base {
	cherokee_handler_t                 handler;

	/* Properties
	 */
	cherokee_handler_cgi_base_phase_t  init_phase;	
	cuint_t                            got_eof;
	char                              *extra_param;

	cherokee_boolean_t                 content_length_set;
	size_t                             content_length;

	cherokee_buffer_t                  xsendfile;
	void                              *file_handler;

	cherokee_buffer_t                  param; 
	cherokee_buffer_t                  param_extra;
	cherokee_buffer_t                  executable;
	cherokee_buffer_t                  data; 

	/* Virtual methods
	 */
	cherokee_handler_cgi_base_add_env_pair_t  add_env_pair;
	cherokee_handler_cgi_base_read_from_cgi_t read_from_cgi;
} ;

#define HDL_CGI_BASE(x)  ((cherokee_handler_cgi_base_t *)(x))


/* Properties data structure
 */
typedef struct {
	cherokee_module_props_t            base;
	cherokee_list_t                    system_env;
	cuint_t                            change_user;
	cherokee_buffer_t                  script_alias;
 	cherokee_boolean_t                 check_file;	
	cherokee_boolean_t                 allow_xsendfile;
	cherokee_boolean_t                 is_error_handler;
	cherokee_boolean_t                 pass_req_headers;
} cherokee_handler_cgi_base_props_t;

#define PROP_CGI_BASE(x)               ((cherokee_handler_cgi_base_props_t *)(x))
#define HANDLER_CGI_BASE_PROPS(x)      (PROP_CGI_BASE (MODULE(x)->props))


/* Base handler methods
 */
ret_t cherokee_handler_cgi_base_init            (cherokee_handler_cgi_base_t              *hdl, 
						 cherokee_connection_t                    *conn,
						 cherokee_plugin_info_handler_t           *info,
						 cherokee_handler_props_t                 *props, 
						 cherokee_handler_cgi_base_add_env_pair_t  add_env_pair,
						 cherokee_handler_cgi_base_read_from_cgi_t read_from_cgi);

ret_t cherokee_handler_cgi_base_free            (cherokee_handler_cgi_base_t *hdl);

ret_t cherokee_handler_cgi_base_add_headers     (cherokee_handler_cgi_base_t *cgi, cherokee_buffer_t *buffer);
ret_t cherokee_handler_cgi_base_step            (cherokee_handler_cgi_base_t *cgi, cherokee_buffer_t *buffer);

void  cherokee_handler_cgi_base_add_parameter   (cherokee_handler_cgi_base_t *cgi, char *name, cuint_t len);
ret_t cherokee_handler_cgi_base_extract_path    (cherokee_handler_cgi_base_t *cgi, cherokee_boolean_t check_filename);
ret_t cherokee_handler_cgi_base_split_pathinfo  (cherokee_handler_cgi_base_t *cgi, 
					  	 cherokee_buffer_t           *buf, 
						 int                          pos,
						 int                          allow_dirs);

ret_t cherokee_handler_cgi_base_build_envp      (cherokee_handler_cgi_base_t *cgi, cherokee_connection_t *conn);
ret_t cherokee_handler_cgi_base_build_basic_env (cherokee_handler_cgi_base_t              *cgi, 
						 cherokee_handler_cgi_base_add_env_pair_t  set_env_pair,
						 cherokee_connection_t                    *conn,
						 cherokee_buffer_t                        *tmp);

ret_t cherokee_handler_cgi_base_configure       (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **props);


/* Handler properties
 */
ret_t cherokee_handler_cgi_base_props_init_base (cherokee_handler_cgi_base_props_t *props, module_func_props_free_t free_func);
ret_t cherokee_handler_cgi_base_props_free      (cherokee_handler_cgi_base_props_t *props);


#endif /* CHEROKEE_HANDLER_CGI_BASE_H */

