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

#ifndef CHEROKEE_PROTOCOL_HTTP_H
#define CHEROKEE_PROTOCOL_HTTP_H

#include "protocol.h"

CHEROKEE_BEGIN_DECLS

ret_t cherokee_protocol_http_dispatch     (cherokee_protocol_t     *proto);

ret_t cherokee_protocol_http_add_response (cherokee_protocol_t     *proto,
					   cherokee_http_version_t  version,
					   cherokee_http_t          status,
					   cherokee_buffer_t       *header);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_PROTOCOL_HTTP_H */
