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

#ifndef CHEROKEE_HANDLER_COMMON_H
#define CHEROKEE_HANDLER_COMMON_H

#include "common-internal.h"

#include "handler_file.h"
#include "handler_dirlist.h"


typedef struct {
	cherokee_handler_props_t          base;
	cherokee_boolean_t                allow_pathinfo;
	cherokee_boolean_t                allow_dirlist;
	cherokee_handler_file_props_t    *props_file;
	cherokee_handler_dirlist_props_t *props_dirlist;
} cherokee_handler_common_props_t;

#define PROP_COMMON(x)  ((cherokee_handler_common_props_t *)(x))


/* Library init function
 */
void  PLUGIN_INIT_NAME(common)           (cherokee_plugin_loader_t *loader);

ret_t cherokee_handler_common_configure  (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **props);
ret_t cherokee_handler_common_new        (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_common_props_free (cherokee_handler_common_props_t *props);

#endif /* CHEROKEE_HANDLER_COMMON_H */
