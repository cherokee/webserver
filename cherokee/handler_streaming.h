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

#ifndef CHEROKEE_HANDLER_STREAMING_H
#define CHEROKEE_HANDLER_STREAMING_H

#include "common-internal.h"

#include "buffer.h"
#include "handler.h"
#include "handler_file.h"
#include "plugin_loader.h"

#ifdef USE_FFMPEG
# include <libavformat/avformat.h>
#endif

/* Data types
 */
typedef struct {
	cherokee_handler_props_t       base;
	cherokee_boolean_t             auto_rate;
	float                          auto_rate_factor;
	cuint_t                        auto_rate_boost;
	cherokee_handler_file_props_t *props_file;
} cherokee_handler_streaming_props_t;

typedef struct {
	cherokee_handler_t             handler;
	cherokee_handler_file_t       *handler_file;
	cherokee_buffer_t              local_file;
#ifdef USE_FFMPEG
	AVFormatContext               *avformat;
#endif
	cint_t                         auto_rate_bps;
	off_t                          start;
	cherokee_boolean_t             start_flv;
	float                          start_time;
	off_t                          boost_until;
} cherokee_handler_streaming_t;


#define PROP_STREAMING(x)      ((cherokee_handler_streaming_props_t *)(x))
#define HDL_STREAMING(x)       ((cherokee_handler_streaming_t *)(x))
#define HDL_STREAMING_PROP(x)  (PROP_STREAMING(MODULE(x)->props))


/* Library init function
 */
void PLUGIN_INIT_NAME(streaming) (cherokee_plugin_loader_t *loader);

ret_t cherokee_handler_streaming_configure  (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **props);
ret_t cherokee_handler_streaming_new        (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_streaming_free       (cherokee_handler_streaming_t *hdl);
ret_t cherokee_handler_streaming_props_free (cherokee_handler_streaming_props_t *props);

/* Virtual methods
 */
ret_t cherokee_handler_streaming_init        (cherokee_handler_streaming_t *hdl);
ret_t cherokee_handler_streaming_add_headers (cherokee_handler_streaming_t *hdl,
                                              cherokee_buffer_t            *buffer);
ret_t cherokee_handler_streaming_step        (cherokee_handler_streaming_t *hdl,
                                              cherokee_buffer_t            *buffer);

#endif /* CHEROKEE_HANDLER_STREAMING_H */
