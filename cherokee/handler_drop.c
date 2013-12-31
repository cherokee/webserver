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
#include "handler_drop.h"
#include "connection-protected.h"

#define ENTRIES "handler,drop"

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (drop, http_all_methods);


ret_t
cherokee_handler_drop_new (cherokee_handler_t     **hdl,
                           cherokee_connection_t   *cnt,
                           cherokee_module_props_t *props)
{
	UNUSED (hdl);
	UNUSED (cnt);
	UNUSED (props);

	return ret_eof;
}

ret_t
cherokee_handler_drop_configure (cherokee_config_node_t   *conf,
                                 cherokee_server_t        *srv,
                                 cherokee_module_props_t **props)
{
	UNUSED(conf);
	UNUSED(srv);
	UNUSED(props);

	return ret_ok;
}
