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

#ifndef CHEROKEE_HANDLER_PROXY_H
#define CHEROKEE_HANDLER_PROXY_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common-internal.h"
#include "handler.h"
#include "downloader.h"
#include "downloader-protected.h"
#include "module_loader.h"
#include "buffer.h"
#include "table.h"
#include "connection.h"
#include "balancer.h"

/* Data types
 */
typedef struct {
	cherokee_module_props_t    base;
	cherokee_balancer_t       *balancer;
} cherokee_handler_proxy_props_t;

typedef struct {
	cherokee_handler_t       handler;	
	cherokee_downloader_t    client;
	cherokee_buffer_t        url;
} cherokee_handler_proxy_t;

#define PROP_PROXY(x)      ((cherokee_handler_proxy_props_t *)(x))
#define HDL_PROXY(x)       ((cherokee_handler_proxy_t *)(p))
#define HDL_PROXY_PROPS(x) (PROP_PROXY(HANDLER(x)->props))


/* Library init function
 */
void  MODULE_INIT(proxy)         (cherokee_module_loader_t *loader);
ret_t cherokee_handler_proxy_new (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);


/* Virtual methods
 */
ret_t cherokee_handler_proxy_init        (cherokee_handler_proxy_t *hdl);
ret_t cherokee_handler_proxy_free        (cherokee_handler_proxy_t *hdl);
void  cherokee_handler_proxy_get_name    (cherokee_handler_proxy_t *hdl, const char **name);
ret_t cherokee_handler_proxy_step        (cherokee_handler_proxy_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_proxy_add_headers (cherokee_handler_proxy_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_PROXY_H */
