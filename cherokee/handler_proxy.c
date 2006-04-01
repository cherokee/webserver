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


cherokee_module_info_handler_t MODULE_INFO(proxy) = {
	.module.type     = cherokee_handler,             /* type         */
	.module.new_func = cherokee_handler_proxy_new,   /* new func     */
	.valid_methods   = http_get | http_head          /* http methods */
};


ret_t
cherokee_handler_proxy_new  (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_table_t *properties)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_proxy); 

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt);

	MODULE(n)->free         = (module_func_free_t) cherokee_handler_proxy_free;
	MODULE(n)->get_name     = (module_func_get_name_t) cherokee_handler_proxy_get_name;
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_proxy_init;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_proxy_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_proxy_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_length | hsupport_range;

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
cherokee_handler_proxy_free (cherokee_handler_proxy_t *n)
{
	/* cleanup 
	 */
	cherokee_buffer_mrproper (&n->url);
	cherokee_downloader_mrproper (&n->client);

	return ret_ok;
}


void  
cherokee_handler_proxy_get_name (cherokee_handler_proxy_t *n, const char **name)
{
	*name = "proxy";
}


ret_t 
cherokee_handler_proxy_init (cherokee_handler_proxy_t *n)
{
	ret_t                  ret;	
	cherokee_connection_t *conn = HANDLER_CONN(n);


	/* Sanity check
	 */
	if (conn->request.len <= 0) 
		return ret_error;

	/* 1. Parse request headers and create a new request headers
	 *    with the necesary header additions/modifications.
	 *    Follow RFC2616
	 */

	/* Construct the URL host + request + pathinfo + query_string
	 */
	ret = cherokee_buffer_add_buffer (&n->url, &conn->host);
	if ( ret != ret_ok ) return ret;
	
	ret = cherokee_buffer_add_buffer (&n->url, &conn->request); 
	if ( ret != ret_ok ) return ret;
	
	ret = cherokee_buffer_add_buffer (&n->url, &conn->pathinfo);
	if ( ret != ret_ok ) return ret;
	
	ret = cherokee_buffer_add_buffer (&n->url, &conn->query_string);
	if ( ret != ret_ok ) return ret;

	/* Fill in the downloader object. Send the request to the remote server
	 */
	ret = cherokee_downloader_set_url(&n->client, &n->url);
	if ( ret != ret_ok ) return ret;
	
	TRACE(ENTRIES, "Request: %s", n->client.request.url.host.buf);
 
	ret = cherokee_downloader_set_keepalive (&n->client, false); 
	if (ret != ret_ok) return ret;

	ret = cherokee_downloader_set_fdpoll (&n->client, CONN_THREAD(conn)->fdpoll, false);
	if (ret != ret_ok) return ret;

	ret = cherokee_downloader_connect (&n->client);
	if (ret != ret_ok) return ret;

	TRACE(ENTRIES, "client->downloader->socket: %d\n",  SOCKET_FD(n->client.socket)); 
	
	return ret_ok;
}



ret_t
cherokee_handler_proxy_add_headers (cherokee_handler_proxy_t *phdl,
				    cherokee_buffer_t        *buffer)
{
	ret_t    ret; 
	cuint_t  end_len; 
	char     *content; 
	cuint_t  len; 
	cherokee_buffer_t *reply_header = &phdl->client.reply_header;

	ret = cherokee_downloader_step (&phdl->client);
	printf ("ret = %d\n", ret);
	
	switch (ret) {
	case ret_ok:
	case ret_eof_have_data:
		break;		
	case ret_eagain:
		printf ("A - fd %d\n", SOCKET_FD(phdl->client.socket));
		cherokee_thread_deactive_to_polling (HANDLER_THREAD(phdl), HANDLER_CONN(phdl), 
						     SOCKET_FD(phdl->client.socket), 0, true);	
		printf ("B - fd %d\n", SOCKET_FD(phdl->client.socket));
		return ret_eagain;
	case ret_eof:
	case ret_error:
		return ret;
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	if (reply_header->len <= 3) {

		return ret_eagain;
	}
	
	printf ("reply_header->len %d\n", reply_header->len);

	/* Look the end of headers
	 */
	content = strstr (reply_header->buf, CRLF CRLF);
	if (content != NULL) {
		end_len = 4;
	} else {
		content = strstr (reply_header->buf, "\n\n");
		end_len = 2;
	}
	
	if (content == NULL) {
		return (phdl->got_eof) ? ret_eof : ret_eagain;
	}

	/* Copy the header
	 */
	len = content - reply_header->buf; 
	printf ("len %d\n", len);

	cherokee_buffer_ensure_size (buffer, len+4);
	cherokee_buffer_add (buffer, reply_header->buf, len);
	cherokee_buffer_add (buffer, CRLF CRLF, 4);

	/* Drop out the headers, we already have a copy
	 */
	cherokee_buffer_move_to_begin (buffer, len + end_len);

	printf ("%d\n", buffer->len);
	printf ("%s\n", buffer->buf);

        /*	
	 * 1. Construct the response header to the client request
	 *    based on the response headers sent by the remote server
	 */
	
	/*
	 * 2. Continue to the next step
	 */
	return ret_ok;
}


ret_t
cherokee_handler_proxy_step (cherokee_handler_proxy_t *phdl, 
			     cherokee_buffer_t       *buffer)
{
	ret_t ret;
	/*
	 * 1. Read the payload of the remote server reply and
	 *    store it in buffer	 
	 */
	
	return ret;
}


/*   Library init function
 */
static cherokee_boolean_t _proxy_is_init = false;

void
MODULE_INIT(proxy) (cherokee_module_loader_t *loader)
{
	/* Init flag
	 */
	if (_proxy_is_init == true) {
		return;
	}
	_proxy_is_init = true;

	/* Init something more..
	 */
}






