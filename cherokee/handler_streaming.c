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
#include "handler_streaming.h"

#include "connection.h"
#include "connection-protected.h"
#include "avl.h"
#include "util.h"

#define ENTRIES    "streaming"
#define FLV_HEADER "FLV\x1\x1\0\0\0\x9\0\0\0\x9"


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
	ret_t                               ret;
	cherokee_list_t                    *i;
	cherokee_handler_streaming_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_streaming_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
		     MODULE_PROPS_FREE(cherokee_handler_streaming_props_free));

		n->props_file       = NULL;
		n->auto_rate        = true;
		n->auto_rate_factor = 0.1;
		n->auto_rate_boost  = 5;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_STREAMING(*_props);

	/* Parse 'streaming' parameters
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "rate")) {
			ret = cherokee_atob (subconf->val.buf, &props->auto_rate);
			if (ret != ret_ok) return ret_error;

		} else if (equal_buf_str (&subconf->key, "rate_factor")) {
			props->auto_rate_factor = strtof (subconf->val.buf, NULL);

		} else if (equal_buf_str (&subconf->key, "rate_boost")) {
			ret = cherokee_atou (subconf->val.buf, &props->auto_rate_boost);
			if (ret != ret_ok) return ret_error;
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
		cherokee_handler_free ((void *) hdl->handler_file);
	}

	cherokee_buffer_mrproper (&hdl->local_file);
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
	cherokee_buffer_init (&n->local_file);

	n->avformat      = NULL;
	n->start         = -1;
	n->start_flv     = false;
	n->start_time    = -1;
	n->auto_rate_bps = -1;
	n->boost_until   = 0;

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}

static ret_t
parse_time_start (cherokee_handler_streaming_t *hdl,
                  cherokee_buffer_t            *value)
{
	float                  start;
	char                  *end    = NULL;
	cherokee_connection_t *conn   = HANDLER_CONN(hdl);

	start = strtof (value->buf, &end);
	if (*end != '\0') {
		goto error;
	}

	if (start < 0) {
		goto error;
	}

	TRACE(ENTRIES, "Starting time: %f\n", start);
	hdl->start_time = start;
	return ret_ok;

error:
	conn->error_code = http_range_not_satisfiable;
	return ret_error;
}


static ret_t
parse_offset_start (cherokee_handler_streaming_t *hdl,
                    cherokee_buffer_t            *value)
{
	long                   start;
	char                  *end    = NULL;
	cherokee_connection_t *conn   = HANDLER_CONN(hdl);

	start = strtol (value->buf, &end, 10);
	if (*end != '\0') {
		goto error;
	}

	if (start < 0) {
		goto error;
	}

	if (start > hdl->handler_file->info->st_size) {
		goto error;
	}

	TRACE(ENTRIES, "Starting point: %d\n", start);
	hdl->start = start;
	return ret_ok;

error:
	conn->error_code = http_range_not_satisfiable;
	return ret_error;
}


static ret_t
seek_mp3 (cherokee_handler_streaming_t *hdl)
{
	int      re;
	ret_t    ret;
	long     timestamp;
	AVPacket pkt;

	/* Calculate timestamp
	*/
	timestamp = hdl->start_time * AV_TIME_BASE;

	if (hdl->avformat->start_time != AV_NOPTS_VALUE) {
		timestamp += hdl->avformat->start_time;
	}

	/* Seek
	*/
	re = av_seek_frame (hdl->avformat,         /* AVFormatContext  */
	                    -1,                    /* Stream index     */
	                    timestamp,             /* target timestamp */
	                    AVSEEK_FLAG_BACKWARD); /* flags            */
	if (re < 0) {
		PRINT_MSG ("WARNING: Couldn't seek: %d\n", re);
		return ret_error;
	}

	/* Read Frame
	*/
	av_init_packet (&pkt);
	av_read_frame (hdl->avformat, &pkt);
	hdl->start = pkt.pos;
	av_free_packet (&pkt);

	/* Seek at the beginning of the frame
	*/
	TRACE(ENTRIES, "Seeking: %d\n", hdl->start);

	ret = cherokee_handler_file_seek (hdl->handler_file, hdl->start);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	return ret_ok;
}

static ret_t
set_rate (cherokee_handler_streaming_t *hdl,
          cherokee_connection_t        *conn,
          long                          rate)
{
	cherokee_handler_streaming_props_t *props = HDL_STREAMING_PROP(hdl);

	if (rate <= 0)
		return ret_ok;

	/* This will be the real limit
	 */
	hdl->auto_rate_bps = rate + (rate * props->auto_rate_factor);

	/* Special 'initial boosting' limit
	 */
	conn->limit_bps = props->auto_rate_boost * hdl->auto_rate_bps;
	conn->limit_rate = true;

	/* How long the initial boost last..
	 */
	if (hdl->start > 0) {
		hdl->boost_until = hdl->start + conn->limit_bps;
	} else {
		hdl->boost_until = conn->limit_bps;
	}

	TRACE(ENTRIES, "Limiting rate (initial boost): %d bytes, until=%d\n",
	      conn->limit_bps, hdl->boost_until);
	return ret_ok;
}


static ret_t
open_media_file (cherokee_handler_streaming_t *hdl)
{
	int re;

	if (hdl->avformat != NULL)
		return ret_ok;

	/* Open the media stream
	 */
	TRACE(ENTRIES, "open_media_file %s\n", hdl->local_file.buf);
	re = avformat_open_input (&hdl->avformat, hdl->local_file.buf, NULL, NULL);
	if (re != 0) {
		goto error;
	}

	/* Read the info
	 */
	re = avformat_find_stream_info (hdl->avformat, NULL);
	if (re < 0) {
		goto error;
	}

	return ret_ok;
error:
	if (hdl->avformat != NULL) {
		TRACE(ENTRIES, "close_file (error) %s\n", hdl->local_file.buf);
		avformat_close_input (&hdl->avformat);
		hdl->avformat = NULL;
	}

	return ret_error;
}


static ret_t
set_auto_rate (cherokee_handler_streaming_t *hdl)
{
	ret_t                  ret;
	long                   rate;
	long                   secs;
	void                  *tmp    = NULL;
	cherokee_connection_t *conn   = HANDLER_CONN(hdl);

	/* Check the cache
	 */
	ret = cherokee_avl_get (&_streaming_cache, &hdl->local_file, &tmp);
	if (ret == ret_ok) {
		rate = POINTER_TO_INT(tmp);

		if (rate <= 0)
			return ret_ok;

		return set_rate (hdl, conn, rate);
	}

	/* Open the media stream
	 */
	ret = open_media_file (hdl);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	/* bits/s to bytes/s
	 */
	rate = (hdl->avformat->bit_rate / 8);
	secs = (hdl->avformat->duration / AV_TIME_BASE);

	TRACE(ENTRIES, "Duration: %d seconds\n", hdl->avformat->duration / AV_TIME_BASE);
	TRACE(ENTRIES, "Rate: %d bps (%d bytes/s)\n", hdl->avformat->bit_rate, rate);

	/* Sanity Check
	 */
	if ((rate < 0) || (secs < 0)) {
		return ret_error;
	}

	if (likely (secs > 0)) {
		long tmp;

		tmp = (hdl->handler_file->info->st_size / secs);
		if (tmp > rate) {
			rate = tmp;
			TRACE(ENTRIES, "New rate: %d bytes/s\n", rate);
		}
	}

	ret = set_rate (hdl, conn, rate);

	cherokee_avl_add (&_streaming_cache,
	                  &hdl->local_file,
	                  INT_TO_POINTER(rate));

	return ret;
}


ret_t
cherokee_handler_streaming_init (cherokee_handler_streaming_t *hdl)
{
	ret_t                               ret;
	cherokee_buffer_t                  *value;
	cherokee_boolean_t                  is_flv = false;
	cherokee_boolean_t                  is_mp3 = false;
	cherokee_buffer_t                  *mime   = NULL;
	cherokee_connection_t              *conn   = HANDLER_CONN(hdl);
	cherokee_handler_streaming_props_t *props  = HDL_STREAMING_PROP(hdl);

	/* Local File
	 */
	cherokee_buffer_add_buffer (&hdl->local_file, &conn->local_directory);
	cherokee_buffer_add_buffer (&hdl->local_file, &conn->request);

	/* Init sub-handler
	 */
	ret = cherokee_handler_file_init (hdl->handler_file);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Media type detection
	 */
	if (hdl->handler_file->mime) {
		cherokee_mime_entry_get_type (hdl->handler_file->mime, &mime);
	}

	if (mime != NULL) {
		if (cherokee_buffer_cmp_str (mime, "video/x-flv") == 0) {
			is_flv = true;
		} else if (cherokee_buffer_cmp_str (mime, "audio/mpeg") == 0) {
			is_mp3 = true;
		}
	}

	/* Parse arguments
	 */
	ret = cherokee_connection_parse_args (conn);
	if (ret == ret_ok) {
		ret = cherokee_avl_get_ptr (conn->arguments, "start", (void **) &value);
		if ((ret == ret_ok) && (value != NULL) && (value->len > 0)) {
			/* Set the starting point
			 */
			if (is_flv) {
				ret = parse_offset_start (hdl, value);
				if (ret != ret_ok)
					return ret_error;

			} else if (is_mp3) {
				ret = parse_time_start (hdl, value);
				if (ret != ret_ok)
					return ret_error;
			}
		}
	}

	/* Seeking
	 */
	if ((is_flv) && (hdl->start > 0)) {
		ret = cherokee_handler_file_seek (hdl->handler_file, hdl->start);
		if (unlikely (ret != ret_ok)) {
			return ret_error;
		}
		hdl->start_flv = true;

	} else if ((is_mp3) && (hdl->start_time > 0)) {
		ret = open_media_file (hdl);
		if (ret != ret_ok) {
			return ret_error;
		}

		ret = seek_mp3 (hdl);
		if (unlikely (ret != ret_ok)) {
			goto out;
		}
	}

	/* Set the Bitrate limit
	 */
	if (props->auto_rate) {
		set_auto_rate (hdl);
	}

out:
	/* Close our ffmpeg handle, all information has been gathered
	 */
	if (hdl->avformat != NULL) {
		TRACE(ENTRIES, "close_file %s\n", hdl->local_file.buf);
		avformat_close_input (&hdl->avformat);
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
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* FLV's "start" parameter
	 */
	if (hdl->start_flv) {
		cherokee_buffer_add_str (buffer, FLV_HEADER);
		hdl->start_flv = false;
		return ret_ok;
	}

	/* Check the initial boost
	 */
	if ((conn->limit_bps > hdl->auto_rate_bps) &&
	    (hdl->handler_file->offset > hdl->boost_until))
	{
		conn->limit_bps  = hdl->auto_rate_bps;
		conn->limit_rate = true;

		TRACE(ENTRIES, "Limiting rate: %d bytes/s\n", conn->limit_bps);
	}

	/* Call the real step
	 */
	return cherokee_handler_file_step (hdl->handler_file, buffer);
}


/* Lock manager
 */

#ifdef HAVE_PTHREAD
static int ff_lockmgr(void **mutex, enum AVLockOp op)
{
	CHEROKEE_MUTEX_T(**pmutex) = (void*)mutex;

	switch (op) {
	case AV_LOCK_CREATE:
		*pmutex = malloc(sizeof(**pmutex));
		CHEROKEE_MUTEX_INIT(*pmutex, NULL);
		break;
	case AV_LOCK_OBTAIN:
		CHEROKEE_MUTEX_LOCK(*pmutex);
		break;
	case AV_LOCK_RELEASE:
		CHEROKEE_MUTEX_UNLOCK(*pmutex);
		break;
	case AV_LOCK_DESTROY:
		CHEROKEE_MUTEX_DESTROY(*pmutex);
		free(*pmutex);
		break;
	}

	return 0;
}
#endif


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
	av_register_all();
#ifdef HAVE_PTHREAD
	av_lockmgr_register(ff_lockmgr);
#endif
}
