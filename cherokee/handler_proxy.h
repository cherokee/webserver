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

#ifndef CHEROKEE_HANDLER_PROXY_H
#define CHEROKEE_HANDLER_PROXY_H

#include "common-internal.h"
#include "buffer.h"
#include "handler.h"
#include "connection.h"
#include "plugin_loader.h"
#include "proxy_hosts.h"
#include "balancer.h"
#include "http.h"
#include "header-protected.h"

/* Data types
 */

typedef enum {
	proxy_init_start,
	proxy_init_get_conn,
	proxy_init_preconnect,
	proxy_init_connect,
	proxy_init_build_headers,
	proxy_init_send_headers,
	proxy_init_send_post,
	proxy_init_read_header
} cherokee_handler_proxy_init_phase_t;

typedef struct {
	cherokee_handler_props_t        base;
	cherokee_balancer_t            *balancer;
	cherokee_handler_proxy_hosts_t  hosts;
	cuint_t                         reuse_max;
	cherokee_boolean_t              vserver_errors;

	/* Request processing */
	cherokee_avl_t                  in_headers_hide;
	cherokee_list_t                 in_headers_add;
	cherokee_list_t                 in_request_regexs;
	cherokee_boolean_t              in_allow_keepalive;
	cherokee_boolean_t              in_preserve_host;

	/* Reply processing */
	cherokee_avl_t                  out_headers_hide;
	cherokee_list_t                 out_headers_add;
	cherokee_list_t                 out_request_regexs;
	cherokee_boolean_t              out_preserve_server;
	cherokee_boolean_t              out_flexible_EOH;
} cherokee_handler_proxy_props_t;

typedef struct {
	cherokee_handler_t              handler;
	cherokee_buffer_t               buffer;
	cherokee_buffer_t               request;
	cherokee_source_t               *src_ref;
	cherokee_handler_proxy_conn_t   *pconn;
	cherokee_buffer_t               tmp;
	cherokee_boolean_t              respinned;
	cherokee_boolean_t              got_all;
	cherokee_boolean_t              resending_post;

	cherokee_handler_proxy_init_phase_t  init_phase;
} cherokee_handler_proxy_t;

#define HDL_PROXY(x)       ((cherokee_handler_proxy_t *)(x))
#define PROP_PROXY(x)      ((cherokee_handler_proxy_props_t *)(x))
#define HDL_PROXY_PROPS(x) (PROP_PROXY(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(proxy)      (cherokee_plugin_loader_t *loader);
ret_t cherokee_handler_proxy_new   (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);

/* virtual methods implementation
 */
ret_t cherokee_handler_proxy_init        (cherokee_handler_proxy_t *hdl);
ret_t cherokee_handler_proxy_free        (cherokee_handler_proxy_t *hdl);
ret_t cherokee_handler_proxy_read_post   (cherokee_handler_proxy_t *hdl);
ret_t cherokee_handler_proxy_add_headers (cherokee_handler_proxy_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_proxy_step        (cherokee_handler_proxy_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_PROXY_H */
