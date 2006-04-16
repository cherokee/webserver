/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
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
#include "handler_error.h"

#include "server.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "header-protected.h"


ret_t 
cherokee_handler_error_new (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_table_t *properties)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_error);
	   
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt);
	HANDLER(n)->support = hsupport_error | hsupport_length;

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_error_init;
	MODULE(n)->free         = (handler_func_free_t) cherokee_handler_error_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_error_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_error_add_headers;
	
	/* Init
	 */
	ret = cherokee_buffer_new (&n->content);
	if (unlikely(ret < ret_ok)) return ret;

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t 
cherokee_handler_error_free (cherokee_handler_error_t *hdl)
{
	cherokee_buffer_free (hdl->content);
	return ret_ok;
}


static ret_t
build_hardcoded_response_page (cherokee_connection_t *cnt, cherokee_buffer_t *buffer)
{
	cuint_t            port;
	cherokee_buffer_t *escaped = NULL;

	cherokee_buffer_add_str (buffer, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" CRLF);
	   
	/* Add page title
	 */
	cherokee_buffer_add_str (buffer, "<html><head><title>");
	cherokee_http_code_copy (cnt->error_code, buffer);

	/* Add big banner
	 */
	cherokee_buffer_add_str (buffer, "</title></head><body><h1>");
	cherokee_http_code_copy (cnt->error_code, buffer);
	cherokee_buffer_add_str (buffer, "</h1>");

	/* Maybe add some info
	 */
	switch (cnt->error_code) {
	case http_not_found:
		if (! cherokee_buffer_is_empty (&cnt->request)) {
			cherokee_buffer_t *req_html_ref = NULL;
			
			if (!cherokee_buffer_is_empty (&cnt->request_original)) 
				cherokee_buffer_escape_set_ref (cnt->request_escape, &cnt->request_original);

			cherokee_buffer_escape_get_html (cnt->request_escape, &req_html_ref);

			cherokee_buffer_ensure_size (buffer, 18 + req_html_ref->len + 30);
			cherokee_buffer_add_str (buffer, "The requested URL ");
			cherokee_buffer_add_buffer (buffer, req_html_ref);
			cherokee_buffer_add_str (buffer, " was not found on this server.");
		}
		break;
	case http_bad_request:
		cherokee_buffer_add_str (buffer, 
					 "Your browser sent a request that this server could not understand.");

		cherokee_buffer_escape_html (cnt->header.input_buffer, &escaped);
		if (escaped == NULL)
			cherokee_buffer_add_va (buffer, "<p><pre>%s</pre>", cnt->header.input_buffer->buf);
		else
			cherokee_buffer_add_va (buffer, "<p><pre>%s</pre>", escaped->buf);
		break;
        case http_access_denied:
		cherokee_buffer_add_str (buffer, "You have no access to the request URL");
		break;
	case http_request_uri_too_long:
		cherokee_buffer_add_str (buffer,
					 "The requested URL's length exceeds the capacity limit for this server.");
		break;		
	case http_moved_permanently:
	case http_moved_temporarily:
		cherokee_buffer_add_va (buffer, 
					"The document has moved <A HREF=\"%s\">here</A>.",
					cnt->redirect.buf);
		break;
	case http_unauthorized:
		cherokee_buffer_add_str (buffer, 
					 "This server could not verify that you are authorized to access the document "
					 "requested.  Either you supplied the wrong credentials (e.g., bad password), "
					 "or your browser doesn't understand how to supply the credentials required.");
		break;
	case http_upgrade_required:
		cherokee_buffer_add_str (buffer,
					 "The requested resource can only be retrieved using SSL.  The server is "
					 "willing to upgrade the current connection to SSL, but your client doesn't "
					 "support it. Either upgrade your client, or try requesting the page "
					 "using https://");
		break;
	default:
		break;
	}
	   
	/* Add page foot
	 */
	cherokee_buffer_add_str (buffer, "<p><hr>");	

	if (cnt->socket.is_tls == non_TLS)
 		port = CONN_SRV(cnt)->port;
	else 
 		port = CONN_SRV(cnt)->port_tls;

	if (CONN_SRV(cnt)->server_token <= cherokee_version_product) {
		cherokee_buffer_add_version (buffer, port, ver_port_html);
	} else {
		cherokee_buffer_add_version (buffer, port, ver_full_html);
	}
	cherokee_buffer_add_str (buffer, "</body></html>"); 

	return ret_ok;
}


ret_t 
cherokee_handler_error_init (cherokee_handler_error_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* Generate the error web page
	 */
	ret = build_hardcoded_response_page (conn, hdl->content);
	if (unlikely(ret < ret_ok)) return ret;

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

	/* Usual headers
	 */
	cherokee_buffer_add_str (buffer, "Content-Type: text/html"CRLF);
	cherokee_buffer_add_va  (buffer, "Content-length: %d"CRLF, hdl->content->len);
	cherokee_buffer_add_str (buffer, "Cache-Control: no-cache"CRLF);
	cherokee_buffer_add_str (buffer, "Pragma: no-cache"CRLF);		
	cherokee_buffer_add_str (buffer, "P3P: CP=3DNOI NID CURa OUR NOR UNI"CRLF);

	return ret_ok;
}


ret_t
cherokee_handler_error_step (cherokee_handler_error_t *hdl, cherokee_buffer_t *buffer)
{
	ret_t ret;

	/* Usual content
	 */
	ret = cherokee_buffer_add_buffer (buffer, hdl->content);
	if (unlikely(ret < ret_ok)) return ret;
	   
	return ret_eof_have_data;
}


