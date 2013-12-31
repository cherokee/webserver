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

#ifndef CHEROKEE_ENCODE_DEFLATE_H
#define CHEROKEE_ENCODE_DEFLATE_H

#include <common.h>

#include "zlib/zlib.h"
#include "module.h"
#include "encoder.h"
#include "connection.h"
#include "plugin_loader.h"

/* Data types
 */
typedef struct {
	cherokee_encoder_props_t base;
	int                      compression_level;
} cherokee_encoder_deflate_props_t;

typedef struct {
	cherokee_encoder_t  base;
	z_stream            stream;
	void               *workspace;
} cherokee_encoder_deflate_t;


#define PROP_DEFLATE(x)     ((cherokee_encoder_deflate_props_t *)(x))
#define ENC_DEFLATE(x)      ((cherokee_encoder_deflate_t *)(x))
#define ENC_DEFLATE_PROP(x) (PROP_DEFLATE(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(deflate)            (cherokee_plugin_loader_t *loader);

/* Methods
 */
ret_t cherokee_encoder_deflate_new         (cherokee_encoder_deflate_t **encoder,
                                            cherokee_encoder_props_t    *props);

ret_t cherokee_encoder_deflate_free        (cherokee_encoder_deflate_t  *encoder);

ret_t cherokee_encoder_deflate_add_headers (cherokee_encoder_deflate_t  *encoder, cherokee_buffer_t *buf);
ret_t cherokee_encoder_deflate_init        (cherokee_encoder_deflate_t  *encoder, cherokee_connection_t *conn);
ret_t cherokee_encoder_deflate_encode      (cherokee_encoder_deflate_t  *encoder, cherokee_buffer_t *in, cherokee_buffer_t *out);
ret_t cherokee_encoder_deflate_flush       (cherokee_encoder_deflate_t  *encoder, cherokee_buffer_t *in, cherokee_buffer_t *out);

#endif /* CHEROKEE_ENCODE_DEFLATE_H */
