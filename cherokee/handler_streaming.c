/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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
#include "handler_streaming.h"

#include "connection.h"
#include "connection-protected.h"
#include "avl.h"
#include "util.h"

#ifdef USE_FFMPEG
# include <libavformat/avformat.h>
#endif

/* Global 'stream rate' cache
 */
static cherokee_avl_t _streaming_cache;


PLUGIN_INFO_HANDLER_EASY_INIT (streaming, http_all_methods);



ret_t
cherokee_handler_streaming_props_free (cherokee_handler_streaming_props_t *props)
{
	if (props->props_file != NULL) {
		cherokee_handler_file_props_free (props->props_file);
		props->props_file = NULL;
	}

	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


ret_t
cherokee_handler_streaming_configure (cherokee_config_node_t   *conf,
				      cherokee_server_t        *srv,
				      cherokee_module_props_t **_props)
{
	cherokee_list_t                    *i;
	cherokee_handler_streaming_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_streaming_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
			MODULE_PROPS_FREE(cherokee_handler_streaming_props_free));

		n->props_file       = NULL;
		n->auto_rate        = true;
		n->auto_rate_factor = 0.01;
		n->auto_rate_boost  = 5;
		
		*_props = MODULE_PROPS(n);
	}

	props = PROP_STREAMING(*_props);

	/* Parse 'streaming' parameters
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);
		
		if (equal_buf_str (&subconf->key, "rate")) {
			props->auto_rate = !! atoi(subconf->key.buf);

		} else if (equal_buf_str (&subconf->key, "rate_factor")) {
			props->auto_rate_factor = strtof (subconf->key.buf, NULL);

		} else if (equal_buf_str (&subconf->key, "rate_boost")) {
			props->auto_rate_boost = atoi (subconf->key.buf);
		}
	}

	/* Parse 'file' parameters
	 */
	return cherokee_handler_file_configure (conf, srv, (cherokee_module_props_t **)&props->props_file);
}


ret_t
cherokee_handler_streaming_free (cherokee_handler_streaming_t *hdl)
{
	if (hdl->handler_file != NULL) {
		cherokee_handler_file_free (hdl->handler_file);
	}

	return ret_ok;
}


ret_t
cherokee_handler_streaming_new (cherokee_handler_t      **hdl,
				void                     *cnt,
				cherokee_module_props_t  *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_streaming);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(streaming));

	MODULE(n)->free         = (module_func_free_t) cherokee_handler_streaming_free;
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_streaming_init;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_streaming_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_streaming_add_headers;

	/* Instance the sub-handler
	 */
	ret = cherokee_handler_file_new ((cherokee_handler_t **)&n->handler_file, cnt,
					 MODULE_PROPS(PROP_STREAMING(props)->props_file));
	if (ret != ret_ok) {
		return ret_ok;
	}

	/* Supported features
	*/
	HANDLER(n)->support = HANDLER(n->handler_file)->support;

	/* Init props
	 */
	n->start         = -1;
	n->auto_rate_bps = -1;

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


static ret_t
parse_start (cherokee_handler_streaming_t *hdl,
	     const char                   *value)
{
	char *end = NULL;

	hdl->start = strtol (value, &end, 10);
	if (*end != '\0') {
		return ret_error;
	}

	if (hdl->start < 0) {
		return ret_error;
	}

	if (hdl->start > hdl->handler_file->info->st_size) {
		return ret_error;
	}

	return ret_ok;
}


#ifdef USE_FFMPEG

static ret_t
set_auto_rate_guts (cherokee_handler_streaming_t *hdl,
		    cherokee_buffer_t            *local_file)
{
	int                                 re;
	ret_t                               ret;
	long                                rate;
	void                               *tmp    = NULL;
	AVFormatContext                    *ic_ptr = NULL;
	cherokee_connection_t              *conn   = HANDLER_CONN(hdl);
	cherokee_handler_streaming_props_t *props  = HDL_STREAMING_PROP(hdl);

	/* Check the cache
	 */
	ret = cherokee_avl_get (&_streaming_cache, local_file, &tmp);
	if (ret == ret_ok) {
		if (rate <= 0)
			return ret_ok;

		rate = POINTER_TO_INT(tmp);
		hdl->auto_rate_bps = rate + (rate * props->auto_rate_factor);

		conn->limit_bps = props->auto_rate_boost * hdl->auto_rate_bps;
		conn->limit_rate = true;

		return ret_ok;
	}

	/* Open the media stream
	 */
	re = av_open_input_file (&ic_ptr, local_file->buf, NULL, 0, NULL);
	if (re != 0) {
		ret = ret_error;
		goto out;
	}

	/* Read the info
	 */
	re = av_find_stream_info (ic_ptr);
	if (re < 0) {
		ret = ret_error;
		goto out;
	}

	/* bits/s to bytes/s
	 */
	rate = ic_ptr->bit_rate / 8;
	if (rate > 0) {
		hdl->auto_rate_bps = rate + (rate * props->auto_rate_factor);

		conn->limit_bps = props->auto_rate_boost * hdl->auto_rate_bps;
		conn->limit_rate = true;
	}

	cherokee_avl_add (&_streaming_cache,
			  local_file,
			  INT_TO_POINTER(rate));

	ret = ret_ok;

out:
	/* Clean up
	 */
	if (ic_ptr) {
		av_close_input_file (ic_ptr);
	}

	return ret;
}
#endif


static ret_t
set_auto_rate (cherokee_handler_streaming_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

#ifdef USE_FFMPEG
	cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);
	ret = set_auto_rate_guts (hdl, &conn->local_directory);
	cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len);

	return ret;
#else
	return ret_ok;
#endif
}


ret_t
cherokee_handler_streaming_init (cherokee_handler_streaming_t *hdl)
{
	ret_t                               ret;
	void                               *value;
	cherokee_buffer_t                  *mime  = NULL;
	cherokee_connection_t              *conn  = HANDLER_CONN(hdl);
	cherokee_handler_streaming_props_t *props = HDL_STREAMING_PROP(hdl);

	/* Init sub-handler
	 */
	ret = cherokee_handler_file_init (hdl->handler_file);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Parse arguments
	 */
	ret = cherokee_connection_parse_args (conn);
	if (ret == ret_ok) {
		ret = cherokee_avl_get_ptr (conn->arguments, "start", &value);
		if (ret == ret_ok) {
			ret = parse_start (hdl, (const char *)value);
			if (ret != ret_ok)
				return ret_error;
		}
	}

	/* Set the Rate
	 */
	if (props->auto_rate) {
		set_auto_rate (hdl);
	}

	/* Check the media type
	 */
	if (hdl->handler_file->mime) {
		cherokee_mime_entry_get_type (hdl->handler_file->mime, &mime);
	}
	
	if (mime != NULL) {
		if (cherokee_buffer_cmp_str (mime, "video/x-flv")) {
			/* TODO
			 */
		}
	}

	return ret_ok;
}


ret_t
cherokee_handler_streaming_add_headers (cherokee_handler_streaming_t *hdl,
					cherokee_buffer_t            *buffer)
{
	ret_t ret;

	/* Add the real headers
	 */
	ret = cherokee_handler_file_add_headers (hdl->handler_file, buffer);

	/* The handler support flags might have changed. Re-sync.
	 */
	HANDLER(hdl)->support = HANDLER(hdl->handler_file)->support;

	return ret;
}

ret_t
cherokee_handler_streaming_step (cherokee_handler_streaming_t *hdl,
				 cherokee_buffer_t            *buffer)
{
	cherokee_connection_t              *conn  = HANDLER_CONN(hdl);
	cherokee_handler_streaming_props_t *props = HDL_STREAMING_PROP(hdl);

	/* Check the initial boost
	 */
	if (conn->limit_bps > hdl->auto_rate_bps) {
		if ((hdl->handler_file->offset - conn->range_start) > 
		    (props->auto_rate_boost * hdl->auto_rate_bps))
		{
			conn->limit_bps  = hdl->auto_rate_bps;
			conn->limit_rate = true;
		}
	}

	/* Call the real step
	 */
	return cherokee_handler_file_step (hdl->handler_file, buffer);
}


/* Library init function
 */
static cherokee_boolean_t _streaming_is_init = false;

void
PLUGIN_INIT_NAME(streaming) (cherokee_plugin_loader_t *loader)
{
	if (_streaming_is_init) return;
	_streaming_is_init = true;

	/* Load the dependences
	 */
	cherokee_plugin_loader_load (loader, "file");

	/* Initialize the global cache
	 */
	cherokee_avl_init (&_streaming_cache);

	/* Initialize FFMpeg
	 */
#ifdef USE_FFMPEG
	av_register_all();
#endif
}
