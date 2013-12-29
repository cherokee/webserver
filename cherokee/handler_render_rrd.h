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

#ifndef CHEROKEE_HANDLER_RENDER_RRD_H
#define CHEROKEE_HANDLER_RENDER_RRD_H

#include "handler.h"
#include "plugin_loader.h"
#include "collector_rrd.h"
#include "handler_file.h"

typedef struct {
	cherokee_handler_props_t       base;
	cherokee_boolean_t             disabled;
	cherokee_handler_file_props_t *file_props;
} cherokee_handler_render_rrd_props_t;

typedef struct {
	cherokee_handler_t             base;
	cherokee_buffer_t              rrd_error;
	cherokee_handler_file_t       *file_hdl;
} cherokee_handler_render_rrd_t;

#define HDL_RENDER_RRD(x)           ((cherokee_handler_render_rrd_t *)(x))
#define PROP_RENDER_RRD(x)          ((cherokee_handler_render_rrd_props_t *)(x))
#define HANDLER_RENDER_RRD_PROPS(x) (PROP_RENDER_RRD(MODULE(x)->props))


/* Library init function
 */
void PLUGIN_INIT_NAME(render_rrd)      (cherokee_plugin_loader_t *loader);

/* Methods
 */
ret_t cherokee_handler_render_rrd_new  (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_render_rrd_free (cherokee_handler_render_rrd_t *hdl);
ret_t cherokee_handler_render_rrd_init (cherokee_handler_render_rrd_t *hdl);

#endif /* CHEROKEE_HANDLER_RENDER_RRD_H */
