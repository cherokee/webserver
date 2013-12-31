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

#include "common-internal.h"
#include "encoder_deflate.h"
#include "plugin_loader.h"
#include "util.h"


/* Plug-in initialization
 */
PLUGIN_INFO_ENCODER_EASIEST_INIT (deflate);


static ret_t
props_free (cherokee_encoder_deflate_t *props)
{
	return cherokee_encoder_props_free_base (ENCODER_PROPS(props));
}

ret_t
cherokee_encoder_deflate_configure (cherokee_config_node_t   *config,
                                    cherokee_server_t        *srv,
                                    cherokee_module_props_t **_props)
{
	ret_t                             ret;
	cherokee_list_t                  *i;
	cherokee_encoder_deflate_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, encoder_deflate_props);

		cherokee_encoder_props_init_base (ENCODER_PROPS(n),
		                                  MODULE_PROPS_FREE(props_free));

		n->compression_level = 4;
		*_props = MODULE_PROPS(n);
	}

	props = PROP_DEFLATE(*_props);

	cherokee_config_node_foreach (i, config) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "compression_level")) {
			ret = cherokee_atoi (subconf->val.buf, &props->compression_level);
			if (unlikely (ret != ret_ok)) {
				return ret_error;
			}
		}
	}

	return cherokee_encoder_configure (config, srv, _props);
}


ret_t
cherokee_encoder_deflate_new (cherokee_encoder_deflate_t **encoder,
                              cherokee_encoder_props_t    *props)
{
	cuint_t workspacesize;
	CHEROKEE_NEW_STRUCT (n, encoder_deflate);

	/* Init
	 */
	cherokee_encoder_init_base (ENCODER(n), PLUGIN_INFO_PTR(deflate), props);

	MODULE(n)->init         = (encoder_func_init_t) cherokee_encoder_deflate_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_encoder_deflate_free;
	ENCODER(n)->add_headers = (encoder_func_add_headers_t) cherokee_encoder_deflate_add_headers;
	ENCODER(n)->encode      = (encoder_func_encode_t) cherokee_encoder_deflate_encode;
	ENCODER(n)->flush       = (encoder_func_flush_t) cherokee_encoder_deflate_flush;

	/* More stuff
	 */
	workspacesize = zlib_deflate_workspacesize();

	n->workspace = malloc (workspacesize);
	if (unlikely (n->workspace == NULL)) {
		free (n);
		return ret_nomem;
	}

	memset (n->workspace, 0, workspacesize);
	memset (&n->stream, 0, sizeof(z_stream));

	/* Return the object
	 */
	*encoder = n;
	return ret_ok;
}


ret_t
cherokee_encoder_deflate_free (cherokee_encoder_deflate_t *encoder)
{
	if (encoder->workspace != NULL) {
		free (encoder->workspace);
	}

	free (encoder);
	return ret_ok;
}


ret_t
cherokee_encoder_deflate_add_headers (cherokee_encoder_deflate_t *encoder,
                                   cherokee_buffer_t       *buf)
{
	UNUSED(encoder);

	cherokee_buffer_ensure_addlen (buf, 50);
	cherokee_buffer_add_str (buf, "Content-Encoding: deflate"CRLF);
	cherokee_buffer_add_str (buf, "Vary: Accept-Encoding"CRLF);

	return ret_ok;
}


static const char *
get_deflate_error_string (int err)
{
	switch (err) {
	case Z_NEED_DICT:     return "Need dict.";
	case Z_ERRNO:         return "Errno";
	case Z_STREAM_ERROR:  return "Stream error";
	case Z_DATA_ERROR:    return "Data error";
	case Z_MEM_ERROR:     return "Memory error";
	case Z_BUF_ERROR:     return "Buffer error";
	case Z_VERSION_ERROR: return "Version error";
	default:
		SHOULDNT_HAPPEN;
	}

	return "unknown";
}


ret_t
cherokee_encoder_deflate_init (cherokee_encoder_deflate_t *encoder,
                               cherokee_connection_t      *conn)
{
	int       err;
	z_stream *z = &encoder->stream;

	UNUSED (conn);

	/* Set the workspace
	 */
	z->workspace = encoder->workspace;

	/* -MAX_WBITS: suppresses zlib header & trailer
	 */
	err = zlib_deflateInit2 (z,
	                         ENC_DEFLATE_PROP(encoder)->compression_level,
	                         Z_DEFLATED,
	                         -MAX_WBITS,
	                         MAX_MEM_LEVEL,
	                         Z_DEFAULT_STRATEGY);

	if (err != Z_OK) {
		LOG_ERROR (CHEROKEE_ERROR_ENCODER_DEFLATEINIT2,
		           get_deflate_error_string(err));

		return ret_error;
	}

	return ret_ok;
}


static ret_t
do_encode (cherokee_encoder_deflate_t *encoder,
           cherokee_buffer_t          *in,
           cherokee_buffer_t          *out,
           int                         flush)
{
	int       err;
	cchar_t   buf[DEFAULT_READ_SIZE];
	z_stream *z = &encoder->stream;

	/* Check size
	 */
	if (in->len <= 0) {
		if (flush != Z_FINISH)
			return ret_ok;

		z->avail_in = 0;
		z->next_in  = NULL;
	} else {
		z->avail_in = in->len;
		z->next_in  = (void *)in->buf;
	}

	/* Compress it
	 */
	do {
		/* Set up fixed-size output buffer.
		 */
		z->next_out  = (Byte *)buf;
		z->avail_out = sizeof(buf);

		err = zlib_deflate (z, flush);

		switch (err) {
		case Z_OK:
			cherokee_buffer_add (out, buf, sizeof(buf) - z->avail_out);
			break;
		case Z_STREAM_END:
			err = zlib_deflateEnd (z);
			if (err != Z_OK) {
				LOG_ERROR (CHEROKEE_ERROR_ENCODER_DEFLATEEND,
				           get_deflate_error_string(err));
				return ret_error;
			}
			cherokee_buffer_add (out, buf, sizeof(buf) - z->avail_out);
			break;
		default:
			LOG_ERROR (CHEROKEE_ERROR_ENCODER_DEFLATE,
			           get_deflate_error_string(err), z->avail_in);

			zlib_deflateEnd (z);
			return ret_error;
		}

	} while (z->avail_out == 0);

	return ret_ok;
}


ret_t
cherokee_encoder_deflate_encode (cherokee_encoder_deflate_t *encoder,
                                 cherokee_buffer_t          *in,
                                 cherokee_buffer_t          *out)
{
	return do_encode (encoder, in, out, Z_PARTIAL_FLUSH);
}


ret_t
cherokee_encoder_deflate_flush (cherokee_encoder_deflate_t *encoder,
                                cherokee_buffer_t          *in,
                                cherokee_buffer_t          *out)
{
	return do_encode (encoder, in, out, Z_FINISH);
}
