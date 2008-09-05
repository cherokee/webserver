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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_HANDLER_H
#define CHEROKEE_HANDLER_H

#include <cherokee/common.h>
#include <cherokee/module.h>
#include <cherokee/buffer.h>


CHEROKEE_BEGIN_DECLS

typedef enum {
	hstat_unset,
	hstat_sending,
	hstat_forbidden,
	hstat_file_not_found,
	hstat_error
} cherokee_handler_status_t;

typedef enum {
	hsupport_nothing       = 0,
	hsupport_length        = 1,         /* Knows the length. Eg: for keep-alive    */
	hsupport_maybe_length  = 1 << 1,    /* It might include content-length         */
	hsupport_range         = 1 << 2,    /* Can handle "Range: bytes=" requests     */
	hsupport_error         = 1 << 3,    /* It is an error handler                  */
	hsupport_full_headers  = 1 << 4,    /* Handler adds the full header stack      */
	hsupport_skip_headers  = 1 << 5     /* The server shouldn't add any headers    */
} cherokee_handler_support_t;


/* Callback function definitions
 */
typedef ret_t (* handler_func_new_t)         (void **handler, void *cnt, cherokee_module_props_t *properties);
typedef ret_t (* handler_func_init_t)        (void  *handler);
typedef ret_t (* handler_func_step_t)        (void  *handler, cherokee_buffer_t *buffer);
typedef ret_t (* handler_func_add_headers_t) (void  *handler, cherokee_buffer_t *buffer);
typedef ret_t (* handler_func_configure_t)   (cherokee_config_node_t *, cherokee_server_t *, cherokee_module_props_t **);


/* Data types
 */
typedef struct {
	cherokee_module_t           module;

	/* Pure virtual methods
	 */
	handler_func_step_t         step;
	handler_func_add_headers_t  add_headers;

	/* Properties
	 */
	void                       *connection;
	cherokee_handler_support_t  support;
} cherokee_handler_t;


#define HANDLER(x)             ((cherokee_handler_t *)(x))
#define HANDLER_CONN(h)        (CONN(HANDLER(h)->connection))
#define HANDLER_SRV(h)         (CONN_SRV(HANDLER_CONN(h)))
#define HANDLER_THREAD(h)      (CONN_THREAD(HANDLER_CONN(h)))
#define HANDLER_SUPPORTS(h,s)  (HANDLER(h)->support & s)


/* Module information
 */
typedef struct {
	cherokee_module_props_t  base;
	cherokee_http_method_t   valid_methods;
} cherokee_handler_props_t;

#define HANDLER_PROPS(x)                   ((cherokee_handler_props_t *)(x))


/* Easy initialization
 */
#define HANDLER_CONF_PROTOTYPE(name)                                \
	ret_t cherokee_handler_ ## name ## _configure (             \
		cherokee_config_node_t   *,                         \
		cherokee_server_t        *,                         \
	 	cherokee_module_props_t **)

#define PLUGIN_INFO_HANDLER_EASY_INIT(name, methods)                \
	HANDLER_CONF_PROTOTYPE(name);                               \
                                                                    \
	PLUGIN_INFO_HANDLER_INIT(name, cherokee_handler,            \
		(void *)cherokee_handler_ ## name ## _new,          \
		(void *)cherokee_handler_ ## name ## _configure,    \
                methods)     

#define PLUGIN_INFO_HANDLER_EASIEST_INIT(name, methods)             \
	PLUGIN_EMPTY_INIT_FUNCTION(name)                            \
	PLUGIN_INFO_HANDLER_EASY_INIT(name, methods)


/* Handler methods
 */
ret_t cherokee_handler_init_base   (cherokee_handler_t  *hdl, void *conn, cherokee_handler_props_t *props, cherokee_plugin_info_handler_t *info);
ret_t cherokee_handler_free_base   (cherokee_handler_t  *hdl);

/* Handler virtual methods
 */
ret_t cherokee_handler_init        (cherokee_handler_t  *hdl);
ret_t cherokee_handler_free        (cherokee_handler_t  *hdl);
ret_t cherokee_handler_step        (cherokee_handler_t  *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_add_headers (cherokee_handler_t  *hdl, cherokee_buffer_t *buffer);

/* Handler properties methods
 */
ret_t cherokee_handler_props_init_base  (cherokee_handler_props_t *props, module_func_props_free_t free_func);
ret_t cherokee_handler_props_free_base  (cherokee_handler_props_t *props);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_HANDLER_H */
