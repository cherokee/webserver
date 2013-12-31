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

#ifndef CHEROKEE_VRULE_H
#define CHEROKEE_VRULE_H

#include <cherokee/common.h>
#include <cherokee/module.h>
#include <cherokee/buffer.h>
#include <cherokee/list.h>
#include <cherokee/plugin.h>

CHEROKEE_BEGIN_DECLS

#define CHEROKEE_VRULE_PRIO_NONE    0
#define CHEROKEE_VRULE_PRIO_DEFAULT 1

/* Callback function definitions
 */
typedef ret_t (* vrule_func_new_t)       (void **vrule);
typedef ret_t (* vrule_func_configure_t) (void  *vrule, cherokee_config_node_t *conf, void *vsrv);
typedef ret_t (* vrule_func_match_t)     (void  *vrule, cherokee_buffer_t *host, void *conn);

/* Data types
 */
typedef struct {
	cherokee_module_t        module;

	/* Properties */
	cherokee_list_t          list_node;
	cuint_t                  priority;
	void                    *virtual_server;

	/* Virtual methods */
	vrule_func_match_t       match;
	vrule_func_configure_t   configure;
} cherokee_vrule_t;

#define VRULE(x) ((cherokee_vrule_t *)(x))

/* Easy initialization
 */
#define PLUGIN_INFO_VRULE_EASY_INIT(name)                         \
	PLUGIN_INFO_INIT(name, cherokee_vrule,                    \
	                 (void *)cherokee_vrule_ ## name ## _new, \
	                 (void *)NULL)

#define PLUGIN_INFO_VRULE_EASIEST_INIT(name)                      \
	PLUGIN_EMPTY_INIT_FUNCTION(name)                          \
	PLUGIN_INFO_VRULE_EASY_INIT(name)

/* Methods
 */
ret_t cherokee_vrule_free        (cherokee_vrule_t *vrule);

/* Vrule methods
 */
ret_t cherokee_vrule_init_base   (cherokee_vrule_t *vrule, cherokee_plugin_info_t *info);

/* Vrule virtual methods
 */
ret_t cherokee_vrule_match       (cherokee_vrule_t *vrule, cherokee_buffer_t *host, void *conn);
ret_t cherokee_vrule_configure   (cherokee_vrule_t *vrule, cherokee_config_node_t *conf, void *vsrv);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_VRULE_H */
