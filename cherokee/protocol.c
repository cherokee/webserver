/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
#include "protocol.h"
#include "connection.h"

ret_t
cherokee_protocol_init  (cherokee_protocol_t *proto, void *cnt)
{
	cherokee_connection_t *conn = CONN(cnt);

	PROTOCOL_BASE(proto)->conn       = cnt;
	PROTOCOL_BASE(proto)->type       = proto_unset;
	PROTOCOL_BASE(proto)->buffer_in  = conn->buffer_in;
	PROTOCOL_BASE(proto)->buffer_out = conn->buffer_out;

	return ret_ok;
}


ret_t
cherokee_protocol_clean (cherokee_protocol_t *proto)
{
	PROTOCOL_BASE(proto)->type       = proto_unset;
	PROTOCOL_BASE(proto)->conn       = NULL;
	PROTOCOL_BASE(proto)->buffer_in  = NULL;
	PROTOCOL_BASE(proto)->buffer_out = NULL;

	return ret_ok;
}


ret_t
cherokee_protocol_mrproper (cherokee_protocol_t *proto)
{
	cherokee_protocol_clean (proto);
	return ret_ok;
}


ret_t
cherokee_protocol_set (cherokee_protocol_t      *proto,
		       cherokee_protocol_type_t  type)
{
	TRACE (ENTRIES, "Setting protocol: %s\n",
	       type == proto_http ? "HTTP" :
	       type == proto_spdy ? "SPDY" : "????");

	PROTOCOL_BASE(proto)->type = type;
	return ret_ok;
}


ret_t
cherokee_protocol_dispatch (cherokee_protocol_t *proto)
{
	switch (PROTOCOL_BASE(proto)->type) {
	case proto_http:
		return cherokee_protocol_http_dispatch (proto);
	case http_spdy:
		return cherokee_protocol_spdy_dispatch (proto);
	default:
		break;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
cherokee_protocol_add_response (cherokee_protocol_t     *proto,
				cherokee_http_version_t  version,
				cherokee_http_t          status,
				cherokee_buffer_t       *header)
{
	switch (PROTOCOL_BASE(proto)->type) {
	case proto_http:
		return cherokee_protocol_http_add_response (proto, version, status, header);
	case http_spdy:
		return cherokee_protocol_spdy_add_response (proto, version, status, header);
	default:
		break;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}
