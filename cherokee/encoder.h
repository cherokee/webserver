/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2013 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_ENCODER_H
#define CHEROKEE_ENCODER_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/module.h>


CHEROKEE_BEGIN_DECLS

typedef enum {
	cherokee_encoder_unset,
	cherokee_encoder_allow,
	cherokee_encoder_forbid
} cherokee_encoder_perms_t;


/* Callback function prototipes
 */
typedef ret_t (* encoder_func_new_t)         (void **encoder, void *props);
typedef ret_t (* encoder_func_free_t)        (void  *encoder);
typedef ret_t (* encoder_func_add_headers_t) (void  *encoder, cherokee_buffer_t *buf);
typedef ret_t (* encoder_func_init_t)        (void  *encoder, void *conn);
typedef ret_t (* encoder_func_encode_t)      (void  *encoder, cherokee_buffer_t *in, cherokee_buffer_t *out);
typedef ret_t (* encoder_func_flush_t)       (void  *encoder, cherokee_buffer_t *in, cherokee_buffer_t *out);
typedef ret_t (* encoder_func_configure_t)   (cherokee_config_node_t *, cherokee_server_t *, cherokee_module_props_t **);


/* Encoder properties
 */
typedef struct {
	cherokee_module_props_t   base;
	cherokee_encoder_perms_t  perms;
	encoder_func_new_t        instance_func;
} cherokee_encoder_props_t;

#define ENCODER_PROPS(x) ((cherokee_encoder_props_t *)(x))


/* Data types
 */
typedef struct {
	cherokee_module_t          module;

	/* Pure virtual methods
	 */
	encoder_func_add_headers_t add_headers;
	encoder_func_encode_t      encode;
	encoder_func_flush_t       flush;

	/* Properties
	 */
	void                      *conn;
} cherokee_encoder_t;

#define ENCODER(x)      ((cherokee_encoder_t *)(x))
#define ENCODER_CONN(x) (CONN(ENCODER(x)->conn))


/* Easy initialization
 */
#define ENCODER_CONF_PROTOTYPE(name)                                \
	ret_t cherokee_encoder_ ## name ## _configure (             \
		cherokee_config_node_t   *,                         \
		cherokee_server_t        *,                         \
	 	cherokee_module_props_t **)

#define PLUGIN_INFO_ENCODER_EASY_INIT(name)                         \
	ENCODER_CONF_PROTOTYPE(name);                               \
                                                                    \
	PLUGIN_INFO_INIT(name, cherokee_encoder,                    \
		(void *)cherokee_encoder_ ## name ## _new,          \
		(void *)cherokee_encoder_ ## name ## _configure)

#define PLUGIN_INFO_ENCODER_EASIEST_INIT(name)                      \
	PLUGIN_EMPTY_INIT_FUNCTION(name)                            \
	PLUGIN_INFO_ENCODER_EASY_INIT(name)


/* Methods
 */
ret_t cherokee_encoder_init_base   (cherokee_encoder_t       *enc,
				    cherokee_plugin_info_t   *info,
				    cherokee_encoder_props_t *props);

/* Base class methods
 */
ret_t cherokee_encoder_configure (cherokee_config_node_t   *config,
				  cherokee_server_t        *srv,
				  cherokee_module_props_t **_props);

ret_t cherokee_encoder_props_init_base (cherokee_encoder_props_t *props,
					module_func_props_free_t  free_func);

ret_t cherokee_encoder_props_free_base (cherokee_encoder_props_t *encoder_props);

/* Encoder virtual methods
 */
ret_t cherokee_encoder_free        (cherokee_encoder_t *enc);
ret_t cherokee_encoder_add_headers (cherokee_encoder_t *enc, cherokee_buffer_t *buf);
ret_t cherokee_encoder_init        (cherokee_encoder_t *enc, void *conn);
ret_t cherokee_encoder_encode      (cherokee_encoder_t *enc, cherokee_buffer_t *in, cherokee_buffer_t *out);
ret_t cherokee_encoder_flush       (cherokee_encoder_t *enc, cherokee_buffer_t *in, cherokee_buffer_t *out);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_ENCODER_H */
