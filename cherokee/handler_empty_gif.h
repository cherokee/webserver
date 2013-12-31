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

#ifndef CHEROKEE_HANDLER_EMPTY_GIF_H
#define CHEROKEE_HANDLER_EMPTY_GIF_H

#include "common-internal.h"
#include "buffer.h"
#include "handler.h"
#include "connection.h"
#include "plugin_loader.h"

/* Data types
 */
typedef struct {
	cherokee_handler_props_t base;
} cherokee_handler_empty_gif_props_t;


typedef struct {
	cherokee_handler_t      handler;
} cherokee_handler_empty_gif_t;


#define PROP_EMPTY_GIF(x)      ((cherokee_handler_empty_gif_props_t *)(x))
#define HDL_EMPTY_GIF(x)       ((cherokee_handler_empty_gif_t *)(x))
#define HDL_EMPTY_GIF_PROP(x)  (PROP_EMPTY_GIF(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(empty_gif)            (cherokee_plugin_loader_t *loader);
ret_t cherokee_handler_empty_gif_new         (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);

/* Virtual methods
 */
ret_t cherokee_handler_empty_gif_init        (cherokee_handler_empty_gif_t *hdl);
ret_t cherokee_handler_empty_gif_free        (cherokee_handler_empty_gif_t *hdl);
ret_t cherokee_handler_empty_gif_step        (cherokee_handler_empty_gif_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_empty_gif_add_headers (cherokee_handler_empty_gif_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_EMPTY_GIF_H */
