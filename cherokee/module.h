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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_MODULE_H
#define CHEROKEE_MODULE_H

#include <cherokee/common.h>
#include <cherokee/plugin.h>
#include <cherokee/config_node.h>
#include <cherokee/server.h>

CHEROKEE_BEGIN_DECLS

/* Callback function prototipes
 */
typedef void   * module_func_init_t;
typedef ret_t (* module_func_new_t)        (void *);
typedef ret_t (* module_func_free_t)       (void *);
typedef ret_t (* module_func_props_free_t) (void *);
typedef ret_t (* module_func_configure_t)  (cherokee_config_node_t *conf, cherokee_server_t *srv, void **props);


/* Module properties
 */
typedef struct {
	module_func_props_free_t  free;
} cherokee_module_props_t;

#define MODULE_PROPS(x)      ((cherokee_module_props_t *)(x))
#define MODULE_PROPS_FREE(f) ((module_func_props_free_t)(f))


/* Data types for module objects
 */
typedef struct {
	cherokee_plugin_info_t   *info;       /* ptr to info structure    */
	cherokee_module_props_t  *props;      /* ptr to local properties  */

	module_func_new_t         instance;   /* constructor              */
	void                     *init;       /* initializer              */
	module_func_free_t        free;       /* destructor               */
} cherokee_module_t;

#define MODULE(x) ((cherokee_module_t *) (x))


/* Methods
 */
ret_t cherokee_module_init_base (cherokee_module_t *module, cherokee_module_props_t *props, cherokee_plugin_info_t *info);
ret_t cherokee_module_get_name  (cherokee_module_t *module, const char **name);

/* Property methods
 */
ret_t cherokee_module_props_init_base (cherokee_module_props_t *prop, module_func_props_free_t free_func);
ret_t cherokee_module_props_free_base (cherokee_module_props_t *prop);
ret_t cherokee_module_props_free      (cherokee_module_props_t *prop);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_MODULE_H */
