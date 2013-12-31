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

#ifndef CHEROKEE_HANDLER_SSI_H
#define CHEROKEE_HANDLER_SSI_H

#include "common-internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "buffer.h"
#include "handler.h"
#include "connection.h"
#include "mime.h"
#include "plugin_loader.h"

/* Data types
 */
typedef struct {
	cherokee_handler_props_t base;
	int                      foo;
} cherokee_handler_ssi_props_t;


typedef struct {
	cherokee_handler_t     handler;

	cherokee_mime_entry_t *mime;
	struct stat            cache_info;
	cherokee_buffer_t      dir;

	cherokee_buffer_t      source;
	cherokee_buffer_t      render;
} cherokee_handler_ssi_t;


#define PROP_SSI(x)      ((cherokee_handler_ssi_props_t *)(x))
#define HDL_SSI(x)       ((cherokee_handler_ssi_t *)(x))
#define HDL_SSI_PROP(x)  (PROP_SSI(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(ssi)            (cherokee_plugin_loader_t *loader);

ret_t cherokee_handler_ssi_new         (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_ssi_configure   (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props);

/* Virtual methods
 */
ret_t cherokee_handler_ssi_init        (cherokee_handler_ssi_t *hdl);
ret_t cherokee_handler_ssi_free        (cherokee_handler_ssi_t *hdl);
ret_t cherokee_handler_ssi_step        (cherokee_handler_ssi_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_ssi_add_headers (cherokee_handler_ssi_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_SSI_H */
