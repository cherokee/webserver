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

#ifndef CHEROKEE_PROTOCOL_H
#define CHEROKEE_PROTOCOL_H

#include "buffer.h"
#include "list.h"

CHEROKEE_BEGIN_DECLS

/* Protocol types */
typedef enum {
	proto_unset,
	proto_http,
	proto_spdy,
} cherokee_protocol_type_t;

/* Base class */
typedef struct {
	cherokee_protocol_type_t  type;
	void                     *conn;
	cherokee_buffer_t        *buffer_in;
	cherokee_buffer_t        *buffer_out;
} cherokee_protocol_base_t;

/* HTTP class
 */
typedef struct {
	cherokee_protocol_base_t base;
	int                      http;
} cherokee_protocol_http_t;

/* SPDY class
 */
typedef struct {
	cherokee_protocol_base_t base;
	int                      http;
} cherokee_protocol_spdy_t;

/* Union
 */
typedef union {
	cherokee_protocol_http_t http;
	cherokee_protocol_spdy_t spdy;
} cherokee_protocol_t;

#define PROTOCOL(p)      ((cherokee_protocol_t *)(p))
#define PROTOCOL_BASE(p) ((cherokee_protocol_base_t *)(p))
#define PROTOCOL_HTTP(p) ((cherokee_protocol_http_t *)(p))
#define PROTOCOL_SPDY(p) ((cherokee_protocol_spdy_t *)(p))

/* Methods */
ret_t cherokee_protocol_init     (cherokee_protocol_t *proto, void *conn);
ret_t cherokee_protocol_clean    (cherokee_protocol_t *proto);
ret_t cherokee_protocol_mrproper (cherokee_protocol_t *proto);

ret_t cherokee_protocol_set      (cherokee_protocol_t *proto, cherokee_protocol_type_t type);
ret_t cherokee_protocol_dispatch (cherokee_protocol_t *proto);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_PROTOCOL_H */
