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

#ifndef CHEROKEE_HANDLER_CGI_H
#define CHEROKEE_HANDLER_CGI_H

#include "common-internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "buffer.h"
#include "handler.h"
#include "list.h"
#include "handler_cgi_base.h"
#include "plugin_loader.h"


#define ENV_VAR_NUM 80


typedef cherokee_handler_cgi_base_props_t cherokee_handler_cgi_props_t;

typedef struct {
	cherokee_handler_cgi_base_t base;

	int               post_data_sent;    /* amount POSTed to the CGI */
	int               pipeInput;         /* read from the CGI */
	int               pipeOutput;        /* write to the CGI */

#ifdef _WIN32
	cherokee_buffer_t envp;
	HANDLE            process;
	HANDLE            thread;
#else
	char             *envp[ENV_VAR_NUM]; /* Environ variables for execve() */
	int               envp_last;
	pid_t             pid;               /* CGI pid */
#endif
} cherokee_handler_cgi_t;

#define HDL_CGI(x)           ((cherokee_handler_cgi_t *)(x))
#define PROP_CGI(x)          ((cherokee_handler_cgi_props_t *)(x))
#define HANDLER_CGI_PROPS(x) (PROP_CGI (MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(cgi)            (cherokee_plugin_loader_t *loader);

/* Methods
 */
ret_t cherokee_handler_cgi_new         (cherokee_handler_t    **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_cgi_free        (cherokee_handler_cgi_t *hdl);

ret_t cherokee_handler_cgi_init        (cherokee_handler_cgi_t *hdl);
ret_t cherokee_handler_cgi_step        (cherokee_handler_cgi_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_cgi_add_headers (cherokee_handler_cgi_t *hdl, cherokee_buffer_t *buffer);


/* This handler export these extra functions to allow phpcgi
 * set enviroment variables, work with pathinfo, etc..
 */
void  cherokee_handler_cgi_add_env_pair   (cherokee_handler_cgi_base_t *cgi, 
					   char *name,    int name_len,
					   char *content, int content_len);

ret_t cherokee_handler_cgi_configure      (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **props);
ret_t cherokee_handler_cgi_props_free     (cherokee_handler_cgi_props_t *props);

#endif /* CHEROKEE_HANDLER_CGI_H */
