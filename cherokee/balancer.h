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

#ifndef CHEROKEE_BALANCER_H
#define CHEROKEE_BALANCER_H

#include <cherokee/common.h>
#include <cherokee/module.h>
#include <cherokee/connection.h>
#include <cherokee/source.h>

#define BAL_DISABLE_TIMEOUT 5*60

CHEROKEE_BEGIN_DECLS

typedef ret_t (* balancer_dispatch_func_t)    (void *balancer, cherokee_connection_t *conn, cherokee_source_t **src);
typedef ret_t (* balancer_report_fail_func_t) (void *balancer, cherokee_connection_t *conn, cherokee_source_t  *src);
typedef ret_t (* balancer_configure_func_t)   (void *balancer, cherokee_server_t *srv, cherokee_config_node_t *conf);

typedef struct {
	cherokee_module_t           module;

	/* Properties */
	cherokee_list_t             entries;
	cuint_t                     entries_len;

	/* Virtual methods */
	balancer_configure_func_t   configure;
	balancer_dispatch_func_t    dispatch;
	balancer_report_fail_func_t report_fail;
} cherokee_balancer_t;

typedef struct {
	cherokee_list_t             listed;
	cherokee_source_t          *source;
	cherokee_boolean_t          disabled;
	time_t                      disabled_until;
} cherokee_balancer_entry_t;


#define BAL(b)       ((cherokee_balancer_t *)(b))
#define BAL_ENTRY(e) ((cherokee_balancer_entry_t *)(e))

typedef ret_t (* balancer_new_func_t)  (cherokee_balancer_t **balancer);
typedef ret_t (* balancer_free_func_t) (cherokee_balancer_t  *balancer);


/* Easy initialization
 */
#define BALANCER_CONF_PROTOTYPE(name)                              \
	ret_t cherokee_balancer_ ## name ## _configure (           \
	        cherokee_balancer_t    *,                          \
	        cherokee_server_t      *,                          \
	        cherokee_config_node_t *)

#define PLUGIN_INFO_BALANCER_EASY_INIT(name)                       \
	BALANCER_CONF_PROTOTYPE(name);                             \
	                                                           \
	PLUGIN_INFO_INIT(name, cherokee_balancer,                  \
	        (void *)cherokee_balancer_ ## name ## _new,        \
	        (void *)cherokee_balancer_ ## name ## _configure)

#define PLUGIN_INFO_BALANCER_EASIEST_INIT(name)                    \
	PLUGIN_EMPTY_INIT_FUNCTION(name)                           \
	PLUGIN_INFO_BALANCER_EASY_INIT(name)


/* Balancer methods
 */
ret_t cherokee_balancer_init_base      (cherokee_balancer_t *balancer, cherokee_plugin_info_t *info);
ret_t cherokee_balancer_configure_base (cherokee_balancer_t *balancer, cherokee_server_t *srv, cherokee_config_node_t *conf);
ret_t cherokee_balancer_mrproper       (cherokee_balancer_t *balancer);
ret_t cherokee_balancer_free           (cherokee_balancer_t *balancer);

/* Virtual methods
 */
ret_t cherokee_balancer_dispatch    (cherokee_balancer_t    *balancer,
                                     cherokee_connection_t  *conn,
                                     cherokee_source_t     **source);

ret_t cherokee_balancer_report_fail (cherokee_balancer_t    *balancer,
                                     cherokee_connection_t  *conn,
                                     cherokee_source_t      *source);

/* Commodity
 */
ret_t cherokee_balancer_instance   (cherokee_buffer_t       *name,
                                    cherokee_config_node_t  *conf,
                                    cherokee_server_t       *srv,
                                    cherokee_balancer_t    **balancer);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_BALANCER_H */
