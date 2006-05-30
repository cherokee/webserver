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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"
#include "handler_proxy.h"
#include "header.h"
#include "thread.h"
#include "header-protected.h"
#include "handler.h"
#include "connection.h"

#include "connection_info.h"
#include "connection-protected.h"
#include "downloader-protected.h"

#define ENTRIES "proxy,handler"


static ret_t 
props_free (cherokee_handler_proxy_props_t *props)
{
	// TODO: Free the local properties 
	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}

ret_t 
cherokee_handler_proxy_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_handler_props_t **_props)
{
	cherokee_handler_proxy_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT(n, handler_proxy_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n), 
						  HANDLER_PROPS_FREE(props_free));		

		// TODO: Init default values here
		*_props = HANDLER_PROPS(n);
	}

	props = PROP_PROXY(*_props);

	// TODOl: Parse the properties tree here

	return ret_ok;
}

ret_t
cherokee_handler_proxy_new  (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_handler_props_t *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_proxy); 

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, props);

	MODULE(n)->free         = (module_func_free_t) cherokee_handler_proxy_free;
	MODULE(n)->get_name     = (module_func_get_name_t) cherokee_handler_proxy_get_name;
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_proxy_init;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_proxy_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_proxy_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_length | hsupport_range | hsupport_dont_add_headers;

	/* Init
	 */
	ret = cherokee_downloader_init(&n->client);
	if ( ret != ret_ok ) return ret;

	ret = cherokee_buffer_init(&n->url);
	if ( ret != ret_ok ) return ret;

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret;
}


ret_t
cherokee_handler_proxy_free (cherokee_handler_proxy_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->url);
	cherokee_downloader_mrproper (&hdl->client);

	return ret_ok;
}


void  
cherokee_handler_proxy_get_name (cherokee_handler_proxy_t *hdl, const char **name)
{
	*name = "proxy";
}


static ret_t 
add_extra_headers (cherokee_handler_proxy_t *hdl)
{
	char                  *i;
	cherokee_connection_t *conn   = HANDLER_CONN(hdl);
	cherokee_buffer_t     *header = conn->header.input_buffer;
	char                  *end    = header->buf + header->len;

	i = strstr (header->buf, CRLF);
	if (i == NULL) return ret_error;

	for (i += 2; i<end;) {
		char *nl; 

		nl = strstr (i, CRLF);
		if (nl == NULL) break;

		if ((strncasecmp (i, "Host:", 5) != 0) &&
		    (strncasecmp (i, "Connection:", 11) != 0)) 
		{
			cherokee_request_header_add_header (&hdl->client.request, i, nl - i);
		}

		i = nl + 2;
	}
	
	return ret_ok;
}


ret_t 
cherokee_handler_proxy_init (cherokee_handler_proxy_t *hdl)
{
	ret_t                  ret;	
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* Sanity check
	 */
	if (conn->request.len <= 0) 
		return ret_error;

	cherokee_connection_parse_args (conn);

	/* Construct the URL host + request + pathinfo + query_string
	 */
	ret = cherokee_buffer_add_buffer (&hdl->url, &conn->host);
	if ( ret != ret_ok ) return ret;
	
	ret = cherokee_buffer_add_buffer (&hdl->url, &conn->request); 
	if ( ret != ret_ok ) return ret;
	
	ret = cherokee_buffer_add_buffer (&hdl->url, &conn->pathinfo);
	if ( ret != ret_ok ) return ret;
	
	if (! cherokee_buffer_is_empty (&conn->query_string)) {
		ret = cherokee_buffer_add (&hdl->url, "?", 1);
		if ( ret != ret_ok ) return ret;

		ret = cherokee_buffer_add_buffer (&hdl->url, &conn->query_string);
		if ( ret != ret_ok ) return ret;
	}

	/* Fill in the downloader object. 
	 */
	ret = cherokee_downloader_set_url(&hdl->client, &hdl->url);
	if ( ret != ret_ok ) return ret;

	TRACE(ENTRIES, "Request: %s\n", hdl->client.request.url.host.buf);

	/* Extra headers
	 */
	ret = add_extra_headers (hdl);
	if ( ret != ret_ok ) return ret;
	
	/* Post
	 */
	if (! cherokee_post_is_empty (&conn->post)) { 
		cherokee_downloader_post_set (&hdl->client, &conn->post);
	}

	/* Properties
	 */
	ret = cherokee_downloader_set_keepalive (&hdl->client, false); 
	if (ret != ret_ok) return ret;

	ret = cherokee_downloader_connect (&hdl->client);
	if (ret != ret_ok) return ret;


	TRACE(ENTRIES, "client->downloader->socket: %d\n",  SOCKET_FD(&hdl->client.socket)); 
	return ret_ok;
}


ret_t
cherokee_handler_proxy_add_headers (cherokee_handler_proxy_t *phdl,
				    cherokee_buffer_t        *buffer)
{
	ret_t              ret; 
	char              *end; 
	cuint_t            skip;
	cherokee_boolean_t rw;
	cherokee_buffer_t *reply_header = &phdl->client.reply_header;

	ret = cherokee_downloader_step (&phdl->client);

	switch (ret) {
	case ret_ok:
	case ret_eof:
	case ret_eof_have_data:
		break;		
	case ret_eagain:
		ret = cherokee_downloader_is_request_sent (&phdl->client);
		rw = (ret == ret_ok) ? 0 : 1;

		cherokee_thread_deactive_to_polling (HANDLER_THREAD(phdl), HANDLER_CONN(phdl), 
						     SOCKET_FD(&phdl->client.socket), rw, false);	
		return ret_eagain;
	case ret_error:
		return ret;
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	if (reply_header->len < 5)
		return ret_eagain;

	end = reply_header->buf + reply_header->len;

	if (strncmp (end - 4, CRLF CRLF, 4) == 0)
		skip = 2;
	else if (strncmp (end - 2, "\n\n", 2) == 0)
		skip = 1;
	else
		return ret_error;

	cherokee_buffer_add (buffer, reply_header->buf, reply_header->len - skip);
	return ret_ok;
}


ret_t
cherokee_handler_proxy_step (cherokee_handler_proxy_t *phdl, 
			     cherokee_buffer_t        *buffer)
{
	ret_t ret;

	ret = cherokee_downloader_step (&phdl->client);

	switch (ret) {
	case ret_ok:
	case ret_eof_have_data:
		break;
	case ret_eagain:
		cherokee_thread_deactive_to_polling (HANDLER_THREAD(phdl), HANDLER_CONN(phdl),
						     SOCKET_FD(&phdl->client.socket), 0, false);
		return ret_eagain;
	case ret_eof:
		break;
		
	case ret_error:
		return ret;
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	if (phdl->client.body.len > 0) {
		cherokee_buffer_swap_buffers (buffer, &phdl->client.body);
		cherokee_buffer_clean (&phdl->client.body);
	}
	
	if ((ret == ret_eof) && (buffer->len > 0))
		return ret_eof_have_data;

	return ret;
}


/*   Library init function
 */
void
MODULE_INIT(proxy) (cherokee_module_loader_t *loader)
{
}

cherokee_module_info_handler_t MODULE_INFO(proxy) = {
	.module.type      = cherokee_handler,                  /* type         */
	.module.new_func  = cherokee_handler_proxy_new,        /* new func     */
	.module.configure = cherokee_handler_proxy_configure,  /* configure    */
	.valid_methods    = http_get | http_post | http_head   /* http methods */
};
