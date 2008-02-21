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

#ifndef CHEROKEE_ENCODE_GZIP_H
#define CHEROKEE_ENCODE_GZIP_H

#include <common.h>

#include "zlib/zlib.h"
#include "module.h"
#include "encoder.h"

/* Data types
 */
typedef struct {
	cherokee_encoder_t  base;

	z_stream            stream;
	void               *workspace;
	cherokee_boolean_t  add_header;
	uLong               crc32;
	uLong               size;
} cherokee_encoder_gzip_t;

#define ENC_GZIP(x) ((cherokee_encoder_gzip_t *)(x))


/* Methods
 */
ret_t cherokee_encoder_gzip_configure   (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **props);

ret_t cherokee_encoder_gzip_new         (cherokee_encoder_gzip_t **encoder);
ret_t cherokee_encoder_gzip_free        (cherokee_encoder_gzip_t  *encoder);

ret_t cherokee_encoder_gzip_add_headers (cherokee_encoder_gzip_t  *encoder, cherokee_buffer_t *buf);
ret_t cherokee_encoder_gzip_init        (cherokee_encoder_gzip_t  *encoder);
ret_t cherokee_encoder_gzip_encode      (cherokee_encoder_gzip_t  *encoder, cherokee_buffer_t *in, cherokee_buffer_t *out);
ret_t cherokee_encoder_gzip_flush       (cherokee_encoder_gzip_t  *encoder, cherokee_buffer_t *in, cherokee_buffer_t *out);

#endif /* CHEROKEE_ENCODE_GZIP_H */
