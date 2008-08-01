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

#ifndef CHEROKEE_HANDLER_FCGI_H
#define CHEROKEE_HANDLER_FCGI_H

#include "common-internal.h"

#include "handler.h"
#include "buffer.h"
#include "plugin_loader.h"
#include "socket.h"
#include "handler_cgi_base.h"
#include "balancer.h"

typedef enum {
	fcgi_post_unknown,
	fcgi_post_init,
	fcgi_post_read,
	fcgi_post_write
} cherokee_handler_fcgi_post_t;

/* Data structure
 */
typedef struct {
	cherokee_handler_cgi_base_t   base;
	cherokee_source_t            *src_ref;
	time_t                        spawned;
	cherokee_socket_t             socket;
	cherokee_handler_fcgi_post_t  post_phase;
	off_t                         post_len;
	cherokee_buffer_t             write_buffer;
} cherokee_handler_fcgi_t;

#define HDL_FCGI(x)  ((cherokee_handler_fcgi_t *)(x))


/* Properties
 */
typedef struct {
	cherokee_handler_cgi_base_t  base;
	cherokee_list_t              server_list; 	
	cherokee_balancer_t         *balancer;
} cherokee_handler_fcgi_props_t;

#define PROP_FCGI(x)          ((cherokee_handler_fcgi_props_t *)(x))
#define HANDLER_FCGI_PROPS(x) (PROP_FCGI (MODULE(x)->props))


 /* Methods
 */
ret_t cherokee_handler_fcgi_new  (cherokee_handler_t     **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_fcgi_free (cherokee_handler_fcgi_t *hdl);
ret_t cherokee_handler_fcgi_init (cherokee_handler_fcgi_t *hdl);

#endif /* CHEROKEE_HANDLER_CGI_H */
