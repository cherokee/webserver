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
#include "request.h"

ret_t
cherokee_request_header_init (cherokee_request_header_t *request)
{
	ret_t ret;

	/* Init the node list information
	 */
	INIT_LIST_HEAD (LIST(&request->list_node));

	/* Set default values
	 */
	request->method    = http_get;
	request->version   = http_version_11;
	request->auth      = http_auth_nothing;
	request->proxy     = false;
	request->keepalive = true;
	request->pipeline  = 1;
	request->post_len  = 0;

	ret = cherokee_url_init (&request->url);
	if (unlikely(ret < ret_ok)) return ret;

	cherokee_buffer_init (&request->extra_headers);
	cherokee_buffer_init (&request->user);
	cherokee_buffer_init (&request->password);

	return ret_ok;
}


ret_t
cherokee_request_header_mrproper (cherokee_request_header_t *request)
{
	cherokee_buffer_mrproper (&request->user);
	cherokee_buffer_mrproper (&request->password);
	cherokee_buffer_mrproper (&request->extra_headers);

	cherokee_url_mrproper (&request->url);
	return ret_ok;
}


ret_t
cherokee_request_header_clean (cherokee_request_header_t *request)
{
	request->method    = http_get;
	request->version   = http_version_11;
	request->keepalive = true;
	request->pipeline  = 1;
	request->post_len  = 0;

	cherokee_url_clean (&request->url);

	return ret_ok;
}

ret_t
cherokee_request_header_uses_proxy (cherokee_request_header_t *request, cherokee_boolean_t proxy)
{
	request->proxy = proxy;
	return ret_ok;
}


ret_t
cherokee_request_header_parse_string (cherokee_request_header_t *request, cherokee_buffer_t *url_string)
{
	return cherokee_url_parse (&request->url, url_string, &request->user, &request->password);
}


ret_t
cherokee_request_header_build_string (cherokee_request_header_t *request, cherokee_buffer_t *buf, cherokee_buffer_t *tmp1, cherokee_buffer_t *tmp2)
{
	ret_t           ret;
	const char     *ptr;
	cuint_t         len;
	cherokee_url_t *url = REQUEST_URL(request);

	/* 200 bytes should be enough for a small header
	 */
	cherokee_buffer_ensure_size (buf, 200);

	/* Add main request line:
	 * GET /dir/object HTTP/1.1
	 */
	ret = cherokee_http_method_to_string (request->method, &ptr, &len);
	if (ret != ret_ok) return ret;

	ret = cherokee_buffer_add (buf, ptr, len);
	if (ret != ret_ok) return ret;

	cherokee_buffer_add_str (buf, " ");

	/* Check if the requests goes through a proxy
	 */
	if (request->proxy) {
		cherokee_buffer_add_str (buf, "http://");
		cherokee_buffer_add_buffer (buf, URL_HOST(url));
	}

	cherokee_buffer_add_buffer (buf, URL_REQUEST(url));

	switch (REQUEST_VERSION(request)) {
	case http_version_11:
		cherokee_buffer_add_str (buf, " HTTP/1.1" CRLF);
		break;
	case http_version_10:
		cherokee_buffer_add_str (buf, " HTTP/1.0" CRLF);
		break;
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	/* Add "Host:" header - in HTTP/1.1
	 */
	if (REQUEST_VERSION(request) == http_version_11) {
		cherokee_buffer_add_str    (buf, "Host: ");
		cherokee_buffer_add_buffer (buf, URL_HOST(url));
		cherokee_buffer_add_str    (buf, CRLF);
	}

	/* Post information
	 */
	if (request->post_len != 0) {
		/* "Content-Length: " FMT_OFFSET CRLF, request->post_len;
		 */
		cherokee_buffer_add_str      (buf, "Content-Length: ");
		cherokee_buffer_add_ullong10 (buf, (cullong_t) request->post_len);
		cherokee_buffer_add_str      (buf, CRLF);
	}

	/* Add "Connection:" header
	 */
	if (REQUEST_KEEPALIVE(request)) {
		cherokee_buffer_add_str (buf, "Connection: Keep-Alive"CRLF);
	} else {
		cherokee_buffer_add_str (buf, "Connection: close"CRLF);
	}

	/* Authentication
	 */
	if (! cherokee_buffer_is_empty (&request->user) ||
	    ! cherokee_buffer_is_empty (&request->password)) {
		cherokee_buffer_clean (tmp1);
		cherokee_buffer_clean (tmp2);

		cherokee_buffer_add_buffer (tmp1, &request->user);
		cherokee_buffer_add_char   (tmp1, ':');
		cherokee_buffer_add_buffer (tmp1, &request->password);

		cherokee_buffer_encode_base64 (tmp1, tmp2);

		cherokee_buffer_add_str    (buf, "Authorization: Basic ");
		cherokee_buffer_add_buffer (buf, tmp2);
		cherokee_buffer_add_str    (buf, CRLF);
	}

	/* Extra headers
	 */
	if (! cherokee_buffer_is_empty (&request->extra_headers)) {
		cherokee_buffer_add_buffer (buf, &request->extra_headers);
	}

	/* Finish the header
	 */
	cherokee_buffer_add_str (buf, CRLF);
	return ret_ok;
}


ret_t
cherokee_request_header_set_auth (cherokee_request_header_t *request,
                                  cherokee_http_auth_t       auth,
                                  cherokee_buffer_t         *user,
                                  cherokee_buffer_t         *password)
{
	request->auth = auth;

	cherokee_buffer_clean (&request->user);
	cherokee_buffer_clean (&request->password);

	cherokee_buffer_add_buffer (&request->user, user);
	cherokee_buffer_add_buffer (&request->password, password);

	return ret_ok;
}


ret_t
cherokee_request_header_add_header (cherokee_request_header_t *request, char *ptr, cuint_t len)
{
	cherokee_buffer_ensure_addlen (&request->extra_headers, len + CSZLEN(CRLF));
	cherokee_buffer_add (&request->extra_headers, ptr, len);
	cherokee_buffer_add_str (&request->extra_headers, CRLF);

	return ret_ok;
}
