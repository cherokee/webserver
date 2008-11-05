/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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
#include "handler_proxy.h"
#include "connection-protected.h"
#include "util.h"


#define DEFAULT_BUF_SIZE (8*1024)  /* 8Kb */

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (proxy, http_all_methods);




static ret_t 
props_free (cherokee_handler_proxy_props_t *props)
{
	cherokee_handler_proxy_hosts_mrproper (&props->hosts);
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


ret_t 
cherokee_handler_proxy_configure (cherokee_config_node_t   *conf,
				  cherokee_server_t        *srv,
				  cherokee_module_props_t **_props)
{
	ret_t                           ret;
	cherokee_list_t                *i;
	cherokee_handler_proxy_props_t *props;

	UNUSED(srv);

	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_proxy_props);

		cherokee_module_props_init_base (MODULE_PROPS(n), 
						 MODULE_PROPS_FREE(props_free));

		n->balancer = NULL;
		cherokee_handler_proxy_hosts_init (&n->hosts);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_PROXY(*_props);

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "balancer")) {
			ret = cherokee_balancer_instance (&subconf->val, subconf, srv, &props->balancer); 
			if (ret != ret_ok) 
				return ret;
		}
	}

	/* Final checks
	 */
	if (props->balancer == NULL) {
		PRINT_ERROR_S ("ERROR: Proxy handler needs a balancer\n");
		return ret_error;
	}

	return ret_ok;
}


static ret_t
build_request (cherokee_handler_proxy_t *hdl,
	       cherokee_buffer_t        *buf)
{
	ret_t                  ret;
	cuint_t                len;
	const char            *str;
	char                  *begin;
	char                  *end;
	char                  *header_end;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* Method */
	ret = cherokee_http_method_to_string (conn->header.method, &str, &len);
	if (ret != ret_ok)
		goto error;

	cherokee_buffer_add (buf, str, len);
	cherokee_buffer_add_char (buf, ' ');

	/* The request */
	ret = cherokee_buffer_add_buffer (buf, &conn->request);
	if (ret != ret_ok)
		goto error;

	ret = cherokee_buffer_add_buffer (buf, &conn->pathinfo);
	if (ret != ret_ok)
		goto error;

	if (! cherokee_buffer_is_empty (&conn->query_string)) {
		cherokee_buffer_add_char (buf, '?');

		ret = cherokee_buffer_add_buffer (buf, &conn->query_string);
		if (ret != ret_ok)
			goto error;
	}

	/* HTTP version */
	ret = cherokee_http_version_to_string (conn->header.version, &str, &len);
	if (ret != ret_ok)
		goto error;

	cherokee_buffer_add_char (buf, ' ');
	cherokee_buffer_add      (buf, str, len);
	cherokee_buffer_add_str  (buf, CRLF);

	/* Header: "Host: " */
	cherokee_buffer_add_str    (buf, "Host: ");
	cherokee_buffer_add_buffer (buf, &hdl->src_ref->host);
	if (hdl->src_ref->port != 80) {
		cherokee_buffer_add_char    (buf, ':');
		cherokee_buffer_add_ulong10 (buf, hdl->src_ref->port);
	}
	cherokee_buffer_add_str (buf, CRLF);

	/* Headers
	 */
	str = strchr (conn->incoming_header.buf, CHR_CR);
	if (str == NULL)
		goto error;

	while ((*str == CHR_CR) || (*str == CHR_LF))
		str++;
	
	/* Add the client headers  */
	begin      = (char *)str;
	header_end = conn->incoming_header.buf + conn->incoming_header.len;

	while ((begin < header_end)) {
		char chr_end;

		/* Where the line ends */
		end = cherokee_header_get_next_line (begin);
		if (end == NULL)
			break;

		chr_end = *end;
		*end = '\0';

		/* Check the header entry  */
		if ((strncmp (begin, "Host:", 5) == 0) ||
		    (strncmp (begin, "Connection:", 11) == 0) ||
		    (strncmp (begin, "Transfer-Encoding:", 18) == 0))
		{
			goto next;
		}

		cherokee_buffer_add     (buf, begin, end-begin);
		cherokee_buffer_add_str (buf, CRLF);
		
		/* Prepare next iteration */
	next:
		*end = chr_end;
		while ((*end == CHR_CR) || (*end == CHR_LF))
			end++;
		begin = end;		
	}	
	

	cherokee_buffer_add_str (buf, CRLF);
	return ret_ok;
error:
	cherokee_buffer_clean (buf);
	return ret_error;
}


ret_t
cherokee_handler_proxy_init (cherokee_handler_proxy_t *hdl)
{
	ret_t                           ret;
	cherokee_handler_proxy_poll_t  *poll;
	cherokee_connection_t          *conn  = HANDLER_CONN(hdl);
	cherokee_handler_proxy_props_t *props = HDL_PROXY_PROPS(hdl);

	switch (hdl->init_phase) {
	case proxy_init_get_conn:
		/* Check with the load balancer
		 */
		if (hdl->src_ref == NULL) {
			ret = cherokee_balancer_dispatch (props->balancer, conn, &hdl->src_ref);
			if (ret != ret_ok)
				return ret;
		}
	
		/* Get the connection poll
		 */	
		ret = cherokee_handler_proxy_hosts_get (&props->hosts,
							hdl->src_ref,
							&poll);
		if (unlikely (ret != ret_ok))
			return ret_error;

		/* Get a connection
		 */
		ret = cherokee_handler_proxy_poll_get (poll, &hdl->pconn, hdl->src_ref);
		if (unlikely (ret != ret_ok))
			return ret_error;
		
		hdl->init_phase = proxy_init_build_headers;

	case proxy_init_build_headers:
		/* Build request
		 */
		ret = build_request (hdl, &hdl->request);
		if (unlikely (ret != ret_ok))
			return ret;

		hdl->init_phase = proxy_init_connect;

	case proxy_init_connect:
		/* Connect to the target host
		 */
		if (! cherokee_socket_is_connected (&hdl->pconn->socket)) {
			ret = cherokee_socket_connect (&hdl->pconn->socket);
			switch (ret) {
			case ret_ok:
				break;
			case ret_deny:
				conn->error_code = http_bad_gateway;
				return ret_error;
			case ret_error:
			case ret_eagain:
				return ret;
			default:
				RET_UNKNOWN(ret);
			}
		}

		hdl->init_phase = proxy_init_send_headers;

	case proxy_init_send_headers:
		/* Send the request
		 */
		ret = cherokee_handler_proxy_conn_send (hdl->pconn, 
							&hdl->request);
		switch (ret) {
		case ret_ok:
			break;
		case ret_error:
		case ret_eagain:
			return ret;
		default:
			RET_UNKNOWN(ret);
		}

		hdl->init_phase = proxy_init_send_post;

	case proxy_init_send_post:
		break;
	}

	return ret_ok;
}


static ret_t
parse_server_header (cherokee_handler_proxy_t *hdl,
		     cherokee_buffer_t        *buf_in,
		     cherokee_buffer_t        *buf_out)
{
	int                      re;
	char                    *p;
	char                    *begin;
	char                    *end;
	char                    *header_end;
	cherokee_http_version_t  version;
	cherokee_connection_t   *conn         = HANDLER_CONN(hdl);

	p = buf_in->buf;
	header_end = buf_in->buf + buf_in->len;

	/* Parse the response line (first line)
	 */
	re = strncmp (p, "HTTP/", 5);
	if (re != 0)
		goto error;
	p+= 5;

	if (strncmp (p, "1.1", 3) == 0) {
		version = http_version_11;
	} else if (strncmp (p, "1.0", 3) == 0) {
		version = http_version_10;
	} else if (strncmp (p, "0.9", 3) == 0) {
		version = http_version_09;
	} else 
		goto error;
	p += 3;
	if (*p != ' ')
		goto error;

	p++;
	if ((p[0] < '0') || (p[0] > '9') ||
	    (p[1] < '0') || (p[1] > '9') ||
	    (p[2] < '0') || (p[2] > '9'))
		goto error;

	re = (int)p[3];
	p[3] = '\0';
	conn->error_code = atoi(p);
	p[3] = (char)re;

	p = strchr (p, CHR_CR);
	while ((*p == CHR_CR) || (*p == CHR_LF)) 
		p++;

	/* Parse the headers
	 */
	begin = p;
	while ((begin < header_end)) {
		char chr_end;

		/* Where the line ends */
		end = cherokee_header_get_next_line (begin);
		if (end == NULL)
			break;

		chr_end = *end;
		*end = '\0';
		
		/* Check the header entry  */
		if ((strncmp (begin, "Connection:", 11) == 0) ||
		    (strncmp (begin, "Transfer-Encoding:", 18) == 0))
		{
			goto next;
		}

		cherokee_buffer_add     (buf_out, begin, end-begin);
		cherokee_buffer_add_str (buf_out, CRLF);
		
		/* Prepare next iteration */
	next:
		*end = chr_end;
		while ((*end == CHR_CR) || (*end == CHR_LF))
			end++;
		begin = end;		
	}	

	return ret_ok;

error:
	conn->error_code = http_version_not_supported;
	return ret_error;
}


ret_t
cherokee_handler_proxy_add_headers (cherokee_handler_proxy_t *hdl,
				    cherokee_buffer_t        *buf)
{
	ret_t ret;
	
	/* Read the client header
	 */
	ret = cherokee_handler_proxy_conn_recv_headers (hdl->pconn,
							&hdl->tmp);
	switch (ret) {
	case ret_ok:
		break;
	case ret_eof:
	case ret_error:
	case ret_eagain:
		return ret;
	default:
		RET_UNKNOWN(ret);
	}

	/* Parse the incoming header
	 */
	ret = parse_server_header (hdl, &hdl->pconn->header_in_raw, buf);
	if (ret != ret_ok)
		return ret;	
	
	return ret_ok;
}


ret_t
cherokee_handler_proxy_step (cherokee_handler_proxy_t *hdl,
			     cherokee_buffer_t        *buf)
{
	ret_t  ret;
	size_t size = 0;

	/* Body read during :add_headers
	 */
	if (! cherokee_buffer_is_empty (&hdl->tmp)) {
		cherokee_buffer_add_buffer (buf, &hdl->tmp);
		cherokee_buffer_clean (&hdl->tmp);
		return ret_ok;
	}

	/* Read from the client
	 */
	ret = cherokee_socket_bufread (&hdl->pconn->socket, buf, buf->size - 2, &size);
	switch (ret) {
	case ret_ok:
	case ret_eof:
	case ret_eagain:
	case ret_error:
		return ret;
	default:
		RET_UNKNOWN(ret);
	}

	return ret_ok;
}



ret_t
cherokee_handler_proxy_new (cherokee_handler_t     **hdl,
			    cherokee_connection_t   *cnt,
			    cherokee_module_props_t *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_proxy);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(proxy));
	   
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_proxy_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_proxy_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_proxy_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_proxy_add_headers;

	HANDLER(n)->support = hsupport_full_headers | hsupport_error;

	/* Init
	 */
	n->pconn        = NULL;
	n->src_ref      = NULL;
	n->init_phase   = proxy_init_get_conn;

	cherokee_buffer_init (&n->tmp);
	cherokee_buffer_init (&n->request);
	cherokee_buffer_init (&n->buffer);

	ret = cherokee_buffer_ensure_size (&n->buffer, DEFAULT_BUF_SIZE);
	if (unlikely(ret != ret_ok)) 
		return ret;
	
	*hdl = HANDLER(n);
	return ret_ok;
}

ret_t
cherokee_handler_proxy_free (cherokee_handler_proxy_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->tmp);
	cherokee_buffer_mrproper (&hdl->buffer);
	cherokee_buffer_mrproper (&hdl->request);
	
	if (hdl->pconn) {
		cherokee_handler_proxy_conn_release (hdl->pconn);
	}

	return ret_ok;
}

