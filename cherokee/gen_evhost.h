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

#ifndef CHEROKEE_VRULE_EVHOST_H
#define CHEROKEE_VRULE_EVHOST_H

#include <cherokee/common.h>
#include <cherokee/module.h>
#include <cherokee/buffer.h>
#include <cherokee/plugin.h>
#include <cherokee/template.h>
#include <cherokee/config_entry.h>
#include <cherokee/plugin_loader.h>

CHEROKEE_BEGIN_DECLS

typedef ret_t (* evhost_func_new_t)       (void **evhost, void *vsrv);
typedef ret_t (* evhost_func_configure_t) (void  *evhost, cherokee_config_node_t *conf);
typedef ret_t (* evhost_func_droot_t)     (void  *evhost, void *conn);

typedef struct {
	/* Object */
	cherokee_module_t   module;

	/* Properties */
	cherokee_template_t tpl_document_root;
	cherokee_boolean_t  check_document_root;

	/* Methods */
	evhost_func_droot_t func_document_root;

} cherokee_generic_evhost_t;

#define EVHOST(x) ((cherokee_generic_evhost_t *)(x))

void  PLUGIN_INIT_NAME(evhost) (cherokee_plugin_loader_t *loader);

ret_t cherokee_generic_evhost_new       (cherokee_generic_evhost_t **evhost);
ret_t cherokee_generic_evhost_configure (cherokee_generic_evhost_t  *evhost,
					 cherokee_config_node_t     *config);
CHEROKEE_END_DECLS

#endif /* CHEROKEE_VRULE_EVHOST_H */
