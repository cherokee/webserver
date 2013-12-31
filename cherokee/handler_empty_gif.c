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
#include "handler_empty_gif.h"
#include "connection-protected.h"

#define ENTRIES "handler,empty_gif"

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (empty_gif, http_get | http_head);

/* 43 chars */
#define HARDCODED_GIF                                            \
	"GIF89a\x01\x00\x01\x00\x80\x01\x00\x00\x00\x00\xff\xff" \
	"\xff\x21\xf9\x04\x01\x00\x00\x01\x00\x2c\x00\x00\x00"   \
	"\x00\x01\x00\x01\x00\x00\x02\x02\x4c\x01\x00\x3B"

ret_t
cherokee_handler_empty_gif_new (cherokee_handler_t     **hdl,
                                cherokee_connection_t   *cnt,
                                cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_empty_gif);

	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(empty_gif));

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_empty_gif_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_empty_gif_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_empty_gif_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_empty_gif_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support = hsupport_length;

	*hdl = HANDLER(n);
	return ret_ok;
}

ret_t
cherokee_handler_empty_gif_step (cherokee_handler_empty_gif_t *hdl,
                                 cherokee_buffer_t            *buffer)
{
	UNUSED(hdl);

	cherokee_buffer_add_str (buffer, HARDCODED_GIF);
	return ret_eof_have_data;
}

ret_t
cherokee_handler_empty_gif_add_headers (cherokee_handler_empty_gif_t *hdl,
                                        cherokee_buffer_t            *buffer)
{
	if (cherokee_connection_should_include_length (HANDLER_CONN(hdl))) {
		cherokee_buffer_add_str (buffer,
		                         "Content-Type: image/gif"CRLF
		                         "Content-Length: 43"CRLF);
	} else {
		cherokee_buffer_add_str (buffer, "Content-Type: image/gif"CRLF);
	}

	return ret_ok;
}


ret_t
cherokee_handler_empty_gif_configure (cherokee_config_node_t   *conf,
                                      cherokee_server_t        *srv,
                                      cherokee_module_props_t **_props)
{
	UNUSED(conf);
	UNUSED(srv);
	UNUSED(_props);

	return ret_ok;
}

ret_t
cherokee_handler_empty_gif_init (cherokee_handler_empty_gif_t *hdl)
{
	UNUSED(hdl);
	return ret_ok;
}

ret_t
cherokee_handler_empty_gif_free (cherokee_handler_empty_gif_t *hdl)
{
	UNUSED(hdl);
	return ret_ok;
}

