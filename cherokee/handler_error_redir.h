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

#ifndef CHEROKEE_HANDLER_ERROR_REDIR_H
#define CHEROKEE_HANDLER_ERROR_REDIR_H

#include "common-internal.h"

#include "handler.h"
#include "connection.h"
#include "plugin_loader.h"

typedef struct {
	cherokee_handler_t handler;
	cherokee_buffer_t *content;
} cherokee_handler_error_redir_t;

typedef struct {
	cherokee_handler_props_t  base;
	cherokee_list_t           errors;
	void                     *error_default;
} cherokee_handler_error_redir_props_t;

#define ERREDIR_HANDLER(x)  ((cherokee_handler_error_redir_t *)(x))
#define PROP_ERREDIR(x)     ((cherokee_handler_error_redir_props_t *)(x))
#define HDL_ERREDIR_PROP(x) (PROP_ERREDIR(HANDLER(x)->props))

/* Library init function
 */
void  PLUGIN_INIT_NAME(error_redir)          (cherokee_plugin_loader_t *loader);

ret_t cherokee_handler_error_redir_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props);
ret_t cherokee_handler_error_redir_new       (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);

#endif /* CHEROKEE_HANDLER_ERROR_REDIR_H */
