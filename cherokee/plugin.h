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

#ifndef CHEROKEE_PLUGIN_H
#define CHEROKEE_PLUGIN_H

#include <cherokee/http.h>


CHEROKEE_BEGIN_DECLS

/* Plug-in types
 */
typedef enum {
	cherokee_generic   = 1,
	cherokee_logger    = 1 << 1,
	cherokee_handler   = 1 << 2,
	cherokee_encoder   = 1 << 3,
	cherokee_validator = 1 << 4,
	cherokee_balancer  = 1 << 5,
	cherokee_rule      = 1 << 6,
	cherokee_vrule     = 1 << 7,
	cherokee_cryptor   = 1 << 8,
	cherokee_collector = 1 << 9
} cherokee_plugin_type_t;


/* Generic Plug-in info structure
 */
typedef struct {
	cherokee_plugin_type_t    type;
	void                     *instance;
	void                     *configure;
	const char               *name;
} cherokee_plugin_info_t;

#define PLUGIN_INFO(x) ((cherokee_plugin_info_t *)(x))


/* Specific Plug-ins structures
 */
typedef struct {
	cherokee_plugin_info_t    plugin;
	cherokee_http_method_t    valid_methods;
} cherokee_plugin_info_handler_t;

typedef struct {
	cherokee_plugin_info_t    plugin;
	cherokee_http_auth_t      valid_methods;
} cherokee_plugin_info_validator_t;

#define PLUGIN_INFO_HANDLER(x)     ((cherokee_plugin_info_handler_t *)(x))
#define PLUGIN_INFO_VALIDATOR(x)   ((cherokee_plugin_info_validator_t *)(x))

/* Commodity macros
 */
#define PLUGIN_INFO_NAME(name)       cherokee_ ## name ## _info
#define PLUGIN_INFO_PTR(name)        PLUGIN_INFO(&PLUGIN_INFO_NAME(name))
#define PLUGIN_INFO_HANDLER_PTR(x)   PLUGIN_INFO_HANDLER(&PLUGIN_INFO_NAME(x))
#define PLUGIN_INFO_VALIDATOR_PTR(x) PLUGIN_INFO_VALIDATOR(&PLUGIN_INFO_NAME(x))


/* Convenience macros
 */
#define PLUGIN_INFO_INIT(name, type, func, conf)                    \
	cherokee_plugin_info_t                                      \
		PLUGIN_INFO_NAME(name) =                            \
		{                                                   \
			type,                                       \
			func,                                       \
			conf,                                       \
			#name                                       \
		}

#define PLUGIN_INFO_HANDLER_INIT(name, type, func, conf, methods)   \
	cherokee_plugin_info_handler_t                              \
		PLUGIN_INFO_NAME(name) = {                          \
		{                                                   \
			type,                                       \
			func,                                       \
			conf,                                       \
			#name                                       \
		},                                                  \
		(methods)                                           \
	}

#define PLUGIN_INFO_VALIDATOR_INIT(name, type, func, conf, methods) \
	cherokee_plugin_info_validator_t                            \
		PLUGIN_INFO_NAME(name) = {                          \
		{                                                   \
			type,                                       \
			func,                                       \
			conf,                                       \
			#name                                       \
		},                                                  \
		(methods)                                           \
	}


/* Easy init macros
 */
#define PLUGIN_INFO_EASY_INIT(type,name)                            \
	PLUGIN_INFO_INIT(name, type,                                \
		(void *)type ## _ ## name ## _new,                  \
		(void *)type ## _ ## name ## _configure)


/* Plugin initialization function
 */
#define PLUGIN_INIT_NAME(name)  cherokee_plugin_ ## name ## _init
#define PLUGIN_IS_INIT(name)    _## name ##_is_init

#define PLUGIN_INIT_PROTOTYPE(name)                                 \
	static cherokee_boolean_t PLUGIN_IS_INIT(name) = false;     \
	void                                                        \
	PLUGIN_INIT_NAME(name) (cherokee_plugin_loader_t *loader)

#define PLUGIN_INIT_ONCE_CHECK(name)                                \
	if (PLUGIN_IS_INIT(name))                                   \
		return;                                             \
	PLUGIN_IS_INIT(name) = true

#define PLUGIN_EMPTY_INIT_FUNCTION(name)                            \
	void                                                        \
	PLUGIN_INIT_NAME(name) (cherokee_plugin_loader_t *loader)   \
	{                                                           \
		UNUSED(loader);                                     \
	}                                                           \


CHEROKEE_END_DECLS

#endif /* CHEROKEE_PLUGIN_H */
