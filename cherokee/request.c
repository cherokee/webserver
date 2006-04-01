/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee Benchmarker
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "common-internal.h"
#include "request.h"

ret_t 
cherokee_request_header_init (cherokee_request_header_t *request)
{
	ret_t ret;

	/* Init the node list information
	 */
	INIT_LIST_HEAD((list_t *)&request->list_entry);
	
	/* Set default values
	 */
	request->method    = http_get;
	request->version   = http_version_11;
	request->keepalive = true;
	request->pipeline  = 1;
	request->post_len  = 0;

	ret = cherokee_url_init (&request->url);
	if (unlikely(ret < ret_ok)) return ret;
	
	return ret_ok;
}


ret_t 
cherokee_request_header_mrproper (cherokee_request_header_t *request)
{
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
cherokee_request_header_build_string (cherokee_request_header_t *request, cherokee_buffer_t *buf)
{
	cherokee_url_t *url = REQUEST_URL(request);

	/* 100 bytes is enought for a small header
	 */
	cherokee_buffer_ensure_size (buf, 100);

	/* Add main request line: 
	 * GET /dir/object HTTP/1.1
	 */
	switch (request->method) {
	case http_get:
		cherokee_buffer_add (buf, "GET ", 4); 		
		break;
	case http_post:
		cherokee_buffer_add (buf, "POST ", 5); 		
		break;
	case http_head:
		cherokee_buffer_add (buf, "HEAD ", 5); 		
		break;
	case http_put:
		cherokee_buffer_add (buf, "PUT ", 4); 		
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	cherokee_buffer_add_buffer (buf, URL_REQUEST(url));

	switch (REQUEST_VERSION(request)) {
	case http_version_11:
		cherokee_buffer_add (buf, " HTTP/1.1" CRLF, 11);
		break;
	case http_version_10:
		cherokee_buffer_add (buf, " HTTP/1.0" CRLF, 11);
		break;
	case http_version_09:
		cherokee_buffer_add (buf, " HTTP/0.9" CRLF, 11);
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	/* Add "Host:" header - in HTTP/1.1
	 */
	if (REQUEST_VERSION(request) == http_version_11) {
		cherokee_buffer_add (buf, "Host: ", 6);
		cherokee_buffer_add_buffer (buf, URL_HOST(url));
		cherokee_buffer_add (buf, CRLF, 2);
	}

	/* Post information
	 */
	if (request->post_len != 0) {

		cherokee_buffer_add_va (buf, "Content-Length: "FMT_OFFSET CRLF, request->post_len);
	}
	
	/* Add "Connection:" header
	 */
	if (REQUEST_KEEPALIVE(request)) {
		cherokee_buffer_add (buf, "Connection: Keep-alive"CRLF, 24); 
	} else {
		cherokee_buffer_add (buf, "Connection: Close"CRLF, 19); 
	}

	/* Authentication
	 */
	if (!cherokee_buffer_is_empty(&url->user) ||
	    !cherokee_buffer_is_empty(&url->passwd)) 
	{
		cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

		cherokee_buffer_add_va (&tmp, "%s:%s", url->user.buf, url->passwd.buf);
		cherokee_buffer_encode_base64 (&tmp);
		
		cherokee_buffer_add_va (buf, "Authorization: Basic %s"CRLF, tmp.buf);

		cherokee_buffer_mrproper (&tmp);
	}

	/* Finish the header
	 */
	cherokee_buffer_add (buf, CRLF, 2);

	return ret_ok;
}
