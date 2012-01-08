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

#ifndef CHEROKEE_CLIENT_REQUEST_H
#define CHEROKEE_CLIENT_REQUEST_H

#include "common.h"

#include <sys/types.h>

#include "url.h"
#include "http.h"

typedef struct {
	cherokee_list_t         list_node;
	cherokee_url_t          url;

	uint16_t                pipeline;
	cherokee_boolean_t      proxy;
	cherokee_boolean_t      keepalive;
	cherokee_http_method_t  method;
	cherokee_http_version_t version;
	off_t                   post_len;
	cherokee_buffer_t       extra_headers;

	cherokee_http_auth_t    auth;
	cherokee_buffer_t       user;
	cherokee_buffer_t       password;
} cherokee_client_request_header_t;


#define CLIENT_REQUEST(r)           ((cherokee_client_request_header_t *)(r))
#define CLIENT_REQUEST_METHOD(r)    (CLIENT_REQUEST(r)->method)
#define CLIENT_REQUEST_VERSION(r)   (CLIENT_REQUEST(r)->version)
#define CLIENT_REQUEST_PIPELINE(r)  (CLIENT_REQUEST(r)->pipeline)
#define CLIENT_REQUEST_KEEPALIVE(r) (CLIENT_REQUEST(r)->keepalive)
#define CLIENT_REQUEST_POST(r)      (CLIENT_REQUEST(r)->post_len)
#define CLIENT_REQUEST_URL(r)       (URL(&CLIENT_REQUEST(r)->url))


ret_t cherokee_client_request_header_init     (cherokee_client_request_header_t *request);
ret_t cherokee_client_request_header_clean    (cherokee_client_request_header_t *request);
ret_t cherokee_client_request_header_mrproper (cherokee_client_request_header_t *request);

ret_t cherokee_client_request_header_parse_string (cherokee_client_request_header_t *request, cherokee_buffer_t *url_string);
ret_t cherokee_client_request_header_build_string (cherokee_client_request_header_t *request, cherokee_buffer_t *buf, cherokee_buffer_t *tmp1, cherokee_buffer_t *tmp2);

ret_t cherokee_client_request_header_uses_proxy   (cherokee_client_request_header_t *request, cherokee_boolean_t proxy);
ret_t cherokee_client_request_header_add_header   (cherokee_client_request_header_t *request, char *ptr, cuint_t len);
ret_t cherokee_client_request_header_set_auth     (cherokee_client_request_header_t *request, cherokee_http_auth_t auth, cherokee_buffer_t *user, cherokee_buffer_t *password);

#endif /* CHEROKEE_CLIENT_REQUEST_H */
