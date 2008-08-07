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

#ifndef CHEROKEE_HANDLER_ERROR_NN_H
#define CHEROKEE_HANDLER_ERROR_NN_H

#include "common-internal.h"
#include "handler.h"
#include "connection.h"
#include "plugin_loader.h"

typedef struct {
	cherokee_handler_t handler;
} cherokee_handler_error_nn_t;

/* Library init function
 */
void  PLUGIN_INIT_NAME(error_nn)     (cherokee_plugin_loader_t *loader);

ret_t cherokee_handler_error_nn_new  (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_error_nn_init (cherokee_handler_t  *hdl);

#endif /* CHEROKEE_HANDLER_ERROR_NN_H */
