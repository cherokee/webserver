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

#include "common-internal.h"
#include "encoder_deflate.h"
#include "plugin_loader.h"

/* Plug-in initialization
 */
PLUGIN_INFO_ENCODER_EASIEST_INIT (deflate);


ret_t 
cherokee_encoder_deflate_new (cherokee_encoder_deflate_t **encoder)
{
	cuint_t workspacesize;
	CHEROKEE_NEW_STRUCT (n, encoder_deflate);

	/* Init 	
	 */
	cherokee_encoder_init_base (ENCODER(n), PLUGIN_INFO_PTR(deflate));

	MODULE(n)->init         = (encoder_func_init_t) cherokee_encoder_deflate_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_encoder_deflate_free;
	ENCODER(n)->add_headers = (encoder_func_add_headers_t) cherokee_encoder_deflate_add_headers;
	ENCODER(n)->encode      = (encoder_func_encode_t) cherokee_encoder_deflate_encode;
	ENCODER(n)->flush       = (encoder_func_flush_t) cherokee_encoder_deflate_flush;

	/* More stuff
	 */
	workspacesize = zlib_deflate_workspacesize();

	n->workspace = malloc (workspacesize);
	if (unlikely (n->workspace == NULL)) 
		return ret_nomem;

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


static char *
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
cherokee_encoder_deflate_init (cherokee_encoder_deflate_t *encoder)
{
	int       err;
	z_stream *z = &encoder->stream;

	/* Set the workspace
	 */
	z->workspace = encoder->workspace;
	
	/* -MAX_WBITS: suppresses zlib header & trailer 
	 */
	err = zlib_deflateInit2 (z, 
				 Z_DEFAULT_COMPRESSION, 
				 Z_DEFLATED, 
				 -MAX_WBITS, 
				 MAX_MEM_LEVEL,
				 Z_DEFAULT_STRATEGY);

	if (err != Z_OK) {
		PRINT_ERROR("Error in deflateInit2() = %s\n", 
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
				PRINT_ERROR("Error in deflateEnd(): err=%s\n", 
					    get_deflate_error_string(err));
				return ret_error;
			}
			cherokee_buffer_add (out, buf, sizeof(buf) - z->avail_out);
			break;
		default:
			PRINT_ERROR("Error in deflate(): err=%s avail=%d\n", 
				    get_deflate_error_string(err), 
				    z->avail_in);
			
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
