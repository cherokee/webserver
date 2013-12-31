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
#include "handler_error.h"

#include "server.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "header-protected.h"


/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (error, http_all_methods);


ret_t
cherokee_handler_error_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	UNUSED(conf);
	UNUSED(srv);
	UNUSED(_props);

	return ret_ok;
}


ret_t
cherokee_handler_error_new (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_error);

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(error));

	/* Supported features
	*/
	HANDLER(n)->support = hsupport_error;

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_error_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_error_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_error_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_error_add_headers;

	/* Init
	 */
	ret = cherokee_buffer_init (&n->content);
	if (unlikely(ret < ret_ok))
		return ret;

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_error_free (cherokee_handler_error_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->content);
	return ret_ok;
}


static ret_t
build_hardcoded_response_page (cherokee_connection_t *conn, cherokee_buffer_t *buffer)
{
	/* Avoid too many reallocations.
	 */
	cherokee_buffer_ensure_addlen (buffer, 1000);

	/* Add document header
	 */
	cherokee_buffer_add_str (buffer, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" CRLF);

	/* Add page title
	 */
	cherokee_buffer_add_str (buffer, "<html>" CRLF "<head><title>");
	cherokee_http_code_copy (conn->error_code, buffer);

	/* Add big banner
	 */
	cherokee_buffer_add_str (buffer, "</title>" CRLF
	                         "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />" CRLF
	                         "</head>" CRLF "<body>" CRLF "<h1>");
	cherokee_http_code_copy (conn->error_code, buffer);
	cherokee_buffer_add_str (buffer, "</h1>" CRLF);

	/* Maybe add some info
	 */
	switch (conn->error_code) {
	case http_not_found:
	case http_gone:
		cherokee_buffer_add_str (buffer, "The requested URL ");
		if (! cherokee_buffer_is_empty (&conn->request_original)) {
			cherokee_buffer_add_escape_html (buffer, &conn->request_original);
		}
		else if (! cherokee_buffer_is_empty (&conn->request)) {
			if (cherokee_connection_use_webdir (conn)) {
				cherokee_buffer_add_buffer (buffer, &conn->web_directory);
			}
			cherokee_buffer_add_escape_html (buffer, &conn->request);
		}

		if (conn->error_code == http_not_found) {
			cherokee_buffer_add_str (buffer,
			    " was not found on this server.");
		} else {
			cherokee_buffer_add_str (buffer,
			    " is no longer available on this server and there is no forwarding address.");
		}
		break;

	case http_bad_request:
		cherokee_buffer_add_str (buffer,
		    "Your browser sent a request that this server could not understand.");
		cherokee_buffer_add_str   (buffer, "<p><pre>");
		cherokee_buffer_add_escape_html (buffer, conn->header.input_buffer);
		cherokee_buffer_add_str   (buffer, "</pre>");
		break;

	case http_access_denied:
		cherokee_buffer_add_str (buffer,
		    "You have no access to the requested URL");
		break;

	case http_request_entity_too_large:
		cherokee_buffer_add_str (buffer,
		    "The length of request entity exceeds the capacity limit for this server.");
		break;

	case http_request_uri_too_long:
		cherokee_buffer_add_str (buffer,
		    "The length of requested URL exceeds the capacity limit for this server.");
		break;

	case http_range_not_satisfiable:
		cherokee_buffer_add_str (buffer,
		    "The requested range was not satisfiable.");
		break;

	case http_moved_permanently:
	case http_moved_temporarily:
		cherokee_buffer_add_str         (buffer, "The document has moved <a href=\"");
		cherokee_buffer_add_escape_html (buffer, &conn->redirect);
		cherokee_buffer_add_str         (buffer, "\">here</a>.");
		break;

	case http_unauthorized:
		cherokee_buffer_add_str (buffer,
		    "This server could not verify that you are authorized to access the requested URL.  "
		    "Either you supplied the wrong credentials (e.g., bad password), "
		    "or your browser doesn't know how to supply the credentials required.");
		break;

	case http_upgrade_required:
		cherokee_buffer_add_str (buffer,
		    "The requested resource can only be retrieved using SSL.  The server is "
		    "willing to upgrade the current connection to SSL, but your client doesn't "
		    "support it. Either upgrade your client, or try requesting the page "
		    "using https://");
		break;

	case http_unset:
		SHOULDNT_HAPPEN;
		break;

	default:
		break;
	}

	/* Add page footer
	 */
	cherokee_buffer_add_str (buffer, CRLF "<p><hr>" CRLF);
	cherokee_buffer_add_buffer (buffer, &CONN_BIND(conn)->server_string_w_port);
	cherokee_buffer_add_str (buffer, CRLF "</body>" CRLF "</html>" CRLF);

	return ret_ok;
}


ret_t
cherokee_handler_error_init (cherokee_handler_error_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* If needed then generate the error web page.
	 * Some HTTP response codes should not include body
	 * because it's forbidden by the RFC.
	 */
	if (http_code_with_body (conn->error_code)) {
		ret = build_hardcoded_response_page (conn, &hdl->content);
		if (unlikely(ret < ret_ok))
			return ret;
	}

	return ret_ok;
}


ret_t
cherokee_handler_error_add_headers (cherokee_handler_error_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* It has special headers on protocol upgrading
	 */
	switch (conn->upgrade) {
	case http_upgrade_nothing:
		break;
	case http_upgrade_tls10:
		cherokee_buffer_add_str (buffer, "Upgrade: TLS/1.0, HTTP/1.1"CRLF);
		break;
	case http_upgrade_http11:
		cherokee_buffer_add_str (buffer, "Upgrade: HTTP/1.1"CRLF);
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	/* 1xx, 204 and 304 (Not Modified) responses have to be managed
	 * by "content" handlers, anyway this test ensures that
	 * it'll never send wrong and unrelated headers in case that
	 * a 1xx, 204 or 304 response is managed by this handler.
	 * 304 responses should only include the
	 * Last-Modified, ETag, Expires and Cache-Control headers.
	 */
	if (!http_code_with_body (conn->error_code)) {
		return ret_ok;
	}

	if (cherokee_connection_should_include_length(conn)) {

		HANDLER(hdl)->support |= hsupport_length;

		if (conn->error_code == http_range_not_satisfiable) {
			/* The handler that attended the request has put the content
			* length in conn->range_end in order to allow it to send the
			* right length to the client.
			*
			* "Content-Range: bytes *" "/" FMT_OFFSET CRLF
			*/
			cherokee_buffer_add_str     (buffer, "Content-Range: bytes */");
			cherokee_buffer_add_ullong10(buffer, (cullong_t)conn->range_end);
			cherokee_buffer_add_str     (buffer, CRLF);
		}

		cherokee_buffer_add_str     (buffer, "Content-Length: ");
		cherokee_buffer_add_ulong10 (buffer, (culong_t) hdl->content.len);
		cherokee_buffer_add_str     (buffer, CRLF);
	}

	/* Usual headers
	 */
	cherokee_buffer_add_str (buffer, "Content-Type: text/html"CRLF);

	if (http_type_400(conn->error_code) ||
	    http_type_500(conn->error_code))
	{
		cherokee_buffer_add_str (buffer, "Cache-Control: no-cache"CRLF);
		cherokee_buffer_add_str (buffer, "Pragma: no-cache"CRLF);
	}

	return ret_ok;
}


ret_t
cherokee_handler_error_step (cherokee_handler_error_t *hdl, cherokee_buffer_t *buffer)
{
	ret_t ret;

	/* Usual content
	 */
	ret = cherokee_buffer_add_buffer (buffer, &hdl->content);
	if (unlikely(ret < ret_ok))
		return ret;

	return ret_eof_have_data;
}

