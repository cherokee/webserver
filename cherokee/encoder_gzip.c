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

#include "common-internal.h"

#include "crc32.h"
#include "encoder_gzip.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "header.h"
#include "util.h"

#define ENTRIES "encoder,gzip"


/* Plug-in initialization
 */
PLUGIN_INFO_ENCODER_EASIEST_INIT (gzip);

/* Specs:
 * GZIP file format specification version 4.3:
 * RFC-1952: http://www.ietf.org/rfc/rfc1952.txt
 */

/* Links:
 * http://leknor.com/code/php/class.gzip_encode.php.txt
 */
/* Some operating systems
 */
enum {
	OS_FAT       =   0,
	OS_UNIX      =   3,
	OS_OS2       =   6,
	OS_MACINTOSH =   7,
	OS_NTFS      =  11,
	OS_UNKNOWN   = 255
};


#define gzip_header_len 10
static unsigned char gzip_header[gzip_header_len] = {0x1F, 0x8B,   /* 16 bits: IDentification     */
						     Z_DEFLATED,   /*  b bits: Compression Method */
						     0,            /*  8 bits: FLags              */
						     0, 0, 0, 0,   /* 32 bits: Modification TIME  */
						     0,            /*  8 bits: Extra Flags        */
						     OS_UNIX};     /*  8 bits: Operating System   */

/* GZIP
 * ====
 * gzip_header (10 bytes) + [gzip_encoder_content] + crc32 (4 bytes) + length (4 bytes)
 *
 */

static ret_t
props_free (cherokee_encoder_gzip_t *props)
{
	return cherokee_encoder_props_free_base (ENCODER_PROPS(props));
}

ret_t
cherokee_encoder_gzip_configure (cherokee_config_node_t   *config,
				 cherokee_server_t        *srv,
				 cherokee_module_props_t **_props)
{
	ret_t                          ret;
	cherokee_list_t               *i;
	cherokee_encoder_gzip_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, encoder_gzip_props);

		cherokee_encoder_props_init_base (ENCODER_PROPS(n),
						  MODULE_PROPS_FREE(props_free));

		n->compression_level = Z_DEFAULT_COMPRESSION;
		n->disable_old_IE    = true;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_GZIP(*_props);

	cherokee_config_node_foreach (i, config) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "compression_level")) {
			ret = cherokee_atoi (subconf->val.buf, &props->compression_level);
			if (unlikely (ret != ret_ok)) {
				return ret_error;
			}

		} else if (equal_buf_str (&subconf->key, "disable_old_IE")) {
			ret = cherokee_atob (subconf->val.buf, &props->disable_old_IE);
			if (unlikely (ret != ret_ok)) {
				return ret_error;
			}
		}
	}

	return cherokee_encoder_configure (config, srv, _props);
}


ret_t
cherokee_encoder_gzip_new (cherokee_encoder_gzip_t  **encoder,
			   cherokee_encoder_props_t  *props)
{
	cuint_t workspacesize;
	CHEROKEE_NEW_STRUCT (n, encoder_gzip);

	/* Init
	 */
	cherokee_encoder_init_base (ENCODER(n), PLUGIN_INFO_PTR(gzip), props);

	MODULE(n)->init         = (encoder_func_init_t) cherokee_encoder_gzip_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_encoder_gzip_free;
	ENCODER(n)->add_headers = (encoder_func_add_headers_t) cherokee_encoder_gzip_add_headers;
	ENCODER(n)->encode      = (encoder_func_encode_t) cherokee_encoder_gzip_encode;
	ENCODER(n)->flush       = (encoder_func_flush_t) cherokee_encoder_gzip_flush;

	/* More stuff
	 */
	n->size       = 0;
	n->crc32      = 0;
	n->add_header = true;

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
cherokee_encoder_gzip_free (cherokee_encoder_gzip_t *encoder)
{
	if (encoder->workspace != NULL) {
		free (encoder->workspace);
	}

	free (encoder);
	return ret_ok;
}


ret_t
cherokee_encoder_gzip_add_headers (cherokee_encoder_gzip_t *encoder,
				   cherokee_buffer_t       *buf)
{
	UNUSED(encoder);

	cherokee_buffer_ensure_addlen (buf, 50);
	cherokee_buffer_add_str (buf, "Content-Encoding: gzip"CRLF);
	cherokee_buffer_add_str (buf, "Vary: Accept-Encoding"CRLF);

	return ret_ok;
}


static const char *
get_gzip_error_string (int err)
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

static cherokee_boolean_t
is_user_agent_IE_16 (cherokee_connection_t *conn)
{
	ret_t     ret;
	char     *m;
	char     *ref     = NULL;
	cuint_t   ref_len = 0;

	/* Get the User-Agent header
	 */
	ret = cherokee_header_get_known (&conn->header, header_user_agent, &ref, &ref_len);
	if ((ret != ret_ok) || (ref == NULL) || (ref_len <= 7)) {
		return false;
	}

	/* MSIE [1-6] */
	m = strncasestrn_s (ref, ref_len, "MSIE ");
	if (m == NULL) {
		return false;
	}

	if ((m[5] >= '1') && (m[5] <= '6')) {
		return true;
	}

	return false;
}


ret_t
cherokee_encoder_gzip_init (cherokee_encoder_gzip_t *encoder,
			    cherokee_connection_t   *conn)
{
	int       err;
	z_stream *z    = &encoder->stream;

	/* Disable GZip for IE 1-6 clients
	 */
	if (likely (ENC_GZIP_PROP(encoder)->disable_old_IE)) {
		if (is_user_agent_IE_16 (conn)) {
			TRACE (ENTRIES, "Disabling encoder: %s\n", "MSIE [1-6] detected\n");
			return ret_deny;
		}
	}

	/* Set the workspace
	 */
	z->workspace = encoder->workspace;

	/* Comment from the PHP source code:
	 * windowBits is passed < 0 to suppress zlib header & trailer
	 */
	err = zlib_deflateInit2 (z,
				 ENC_GZIP_PROP(encoder)->compression_level,
				 Z_DEFLATED,
				 -MAX_WBITS,
				 MAX_MEM_LEVEL,
				 Z_DEFAULT_STRATEGY);

	if (err != Z_OK) {
		LOG_ERROR (CHEROKEE_ERROR_ENCODER_DEFLATEINIT2,
			   get_gzip_error_string(err));

		return ret_error;
	}

	return ret_ok;
}



/* "A gzip file consists of a series of "members" (compressed data
 *  sets).  The format of each member is specified in the following
 *  section.  The members simply appear one after another in the file,
 *  with no additional information before, between, or after them."
 *
 * The step() method build a "member":
 */

/* From OpenSSH:
 * Compresses the contents of input_buffer into output_buffer.  All packets
 * compressed using this function will form a single compressed data stream;
 * however, data will be flushed at the end of every call so that each
 * output_buffer can be decompressed independently (but in the appropriate
 * order since they together form a single compression stream) by the
 * receiver.  This appends the compressed data to the output buffer.
 */
static ret_t
do_encode (cherokee_encoder_gzip_t *encoder,
	   cherokee_buffer_t       *in,
	   cherokee_buffer_t       *out,
	   int                      flush)
{
	int       err;
	cchar_t   buf[DEFAULT_READ_SIZE];
	z_stream *z = &encoder->stream;

	/* Update crc32 and size if needed
	 */
	if (in->len <= 0) {
		if (flush != Z_FINISH)
			return ret_ok;

		z->avail_in = 0;
		z->next_in  = NULL;
	} else {
		z->avail_in = in->len;
		z->next_in  = (void *)in->buf;

		encoder->size += in->len;
		encoder->crc32 = crc32_partial_sz (encoder->crc32, in->buf, in->len);
	}

	/* Add the GZip header:
	 * +---+---+---+---+---+---+---+---+---+---+
         * |ID1|ID2|CM |FLG|     MTIME     |XFL|OS |
         * +---+---+---+---+---+---+---+---+---+---+
	 */
	if (encoder->add_header) {
		cherokee_buffer_add (out, (const char *)gzip_header, gzip_header_len);
		encoder->add_header = false;
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
					   get_gzip_error_string(err));

				return ret_error;
			}

			cherokee_buffer_add (out, buf, sizeof(buf) - z->avail_out);
			break;
		default:
			LOG_ERROR (CHEROKEE_ERROR_ENCODER_DEFLATE,
				   get_gzip_error_string(err), z->avail_in);

			zlib_deflateEnd (z);
			return ret_error;
		}

	} while (z->avail_out == 0);

	return ret_ok;
}



ret_t
cherokee_encoder_gzip_encode (cherokee_encoder_gzip_t *encoder,
			      cherokee_buffer_t       *in,
			      cherokee_buffer_t       *out)
{
	return do_encode (encoder, in, out, Z_PARTIAL_FLUSH);
}

ret_t
cherokee_encoder_gzip_flush (cherokee_encoder_gzip_t *encoder, cherokee_buffer_t *in, cherokee_buffer_t *out)
{
	ret_t ret;
	char  footer[8];

 	ret = do_encode (encoder, in, out, Z_FINISH);
 	if (unlikely (ret != ret_ok)) {
 		return ret;
 	}

	/* Add the ending:
	 * +---+---+---+---+---+---+---+---+
	 * |     CRC32     |     ISIZE     |
         * +---+---+---+---+---+---+---+---+
	 */
	footer[0] = (char)  encoder->crc32        & 0xFF;
	footer[1] = (char) (encoder->crc32 >> 8)  & 0xFF;
	footer[2] = (char) (encoder->crc32 >> 16) & 0xFF;
	footer[3] = (char) (encoder->crc32 >> 24) & 0xFF;

	footer[4] = (char)  encoder->size         & 0xFF;
	footer[5] = (char) (encoder->size >> 8)   & 0xFF;
	footer[6] = (char) (encoder->size >> 16)  & 0xFF;
	footer[7] = (char) (encoder->size >> 24)  & 0xFF;

	cherokee_buffer_add (out, footer, 8);

#if 0
 	cherokee_buffer_print_debug (out, -1);
#endif

	return ret_ok;
}

