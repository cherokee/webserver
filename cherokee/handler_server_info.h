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

#ifndef CHEROKEE_HANDLER_SERVER_INFO_H
#define CHEROKEE_HANDLER_SERVER_INFO_H

#include "common-internal.h"
#include "buffer.h"
#include "handler.h"
#include "connection.h"
#include "plugin_loader.h"


/* Data types
 */
typedef struct {
	cherokee_module_props_t  base;
	cherokee_boolean_t       just_about;
	cherokee_boolean_t       connection_details;
} cherokee_handler_server_info_props_t;

typedef struct {
	cherokee_handler_t       handler;
	cherokee_buffer_t        buffer;

	enum {
		send_page,
		send_logo
	} action;
	   
} cherokee_handler_server_info_t;

#define HDL_SRV_INFO(x)       ((cherokee_handler_server_info_t *)(x))
#define PROP_SRV_INFO(x)      ((cherokee_handler_server_info_props_t *)(x))
#define HDL_SRV_INFO_PROPS(x) (PROP_SRV_INFO(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(server_info)      (cherokee_plugin_loader_t *loader);
ret_t cherokee_handler_server_info_new   (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);

/* virtual methods implementation
 */
ret_t cherokee_handler_server_info_init        (cherokee_handler_server_info_t *hdl);
ret_t cherokee_handler_server_info_free        (cherokee_handler_server_info_t *hdl);
ret_t cherokee_handler_server_info_step        (cherokee_handler_server_info_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_server_info_add_headers (cherokee_handler_server_info_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_SERVER_INFO_H */
