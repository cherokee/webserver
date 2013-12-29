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

#ifndef CHEROKEE_HANDLER_SECDOWNLOAD_H
#define CHEROKEE_HANDLER_SECDOWNLOAD_H

#include "common-internal.h"
#include "handler_file.h"


/* Data types
 */
typedef struct {
	cherokee_handler_props_t       base;
	cherokee_handler_file_props_t *props_file;
	cuint_t                        timeout;
	cherokee_buffer_t              secret;
} cherokee_handler_secdownload_props_t;

typedef struct {
	cherokee_handler_t      handler;
	cherokee_boolean_t      use_previous_match;
} cherokee_handler_secdownload_t;

#define PROP_SECDOWN(x)      ((cherokee_handler_secdownload_props_t *)(x))
#define HDL_SECDOWN(x)       ((cherokee_handler_secdownload_t *)(x))
#define HDL_SECDOWN_PROPS(x) (PROP_SECDOWN(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(secdownload) (cherokee_plugin_loader_t *loader);

ret_t cherokee_handler_secdownload_configure  (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **props);
ret_t cherokee_handler_secdownload_new        (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_secdownload_props_free (cherokee_handler_secdownload_props_t *props);

#endif /* CHEROKEE_HANDLER_SECDOWNLOAD_H */
