/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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
#include "handler_proxy.h"
#include "connection-protected.h"
#include "util.h"
#include "thread.h"
#include "server-protected.h"
#include "source_interpreter.h"

#define ENTRIES "proxy"

#define DEFAULT_BUF_SIZE  (64*1024)  /* 64Kb */
#define DEFAULT_REUSE_MAX 16

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (proxy, http_all_methods);


typedef struct {
	cherokee_list_t   listed;
	cherokee_buffer_t key;
	cherokee_buffer_t val;
} cherokee_header_add_t;

#define HEADER_ADD(h) ((cherokee_header_add_t *)(h))

static ret_t
header_add_new (cherokee_header_add_t **header)
{
	CHEROKEE_NEW_STRUCT(n,header_add);

	INIT_LIST_HEAD(&n->listed);
	cherokee_buffer_init (&n->key);
	cherokee_buffer_init (&n->val);

	*header = n;
	return ret_ok;
}

static ret_t
header_add_free (cherokee_header_add_t *header)
{
	cherokee_buffer_mrproper (&header->key);
	cherokee_buffer_mrproper (&header->val);

	free (header);
	return ret_ok;
}



static ret_t 
props_free (cherokee_handler_proxy_props_t *props)
{
	cherokee_list_t *i, *tmp;

	cherokee_handler_proxy_hosts_mrproper (&props->hosts);

	cherokee_avl_mrproper (&props->in_headers_hide, NULL);
	cherokee_avl_mrproper (&props->out_headers_hide, NULL);

	cherokee_regex_list_mrproper (&props->in_request_regexs);
	cherokee_regex_list_mrproper (&props->out_request_regexs);

	list_for_each_safe (i, tmp, &props->in_headers_add) {
		cherokee_list_del (i);
		header_add_free (HEADER_ADD(i));
	}

	list_for_each_safe (i, tmp, &props->out_headers_add) {
		cherokee_list_del (i);
		header_add_free (HEADER_ADD(i));
	}

	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


ret_t 
cherokee_handler_proxy_configure (cherokee_config_node_t   *conf,
				  cherokee_server_t        *srv,
				  cherokee_module_props_t **_props)
{
	ret_t                           ret;
	cherokee_list_t                *i, *j;
	cherokee_handler_proxy_props_t *props;

	UNUSED(srv);

	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_proxy_props);

		cherokee_module_props_init_base (MODULE_PROPS(n), 
						 MODULE_PROPS_FREE(props_free));

		n->balancer            = NULL;
		n->reuse_max           = DEFAULT_REUSE_MAX;
		n->in_allow_keepalive  = true;
		n->in_preserve_host    = false;
		n->out_preserve_server = false;

		INIT_LIST_HEAD (&n->in_request_regexs);
		INIT_LIST_HEAD (&n->in_headers_add);

		INIT_LIST_HEAD (&n->out_headers_add);
		INIT_LIST_HEAD (&n->out_request_regexs);

		cherokee_avl_init (&n->in_headers_hide);
		cherokee_avl_set_case (&n->in_headers_hide, false);

		cherokee_avl_init (&n->out_headers_hide);
		cherokee_avl_set_case (&n->out_headers_hide, false);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_PROXY(*_props);

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "balancer")) {
			ret = cherokee_balancer_instance (&subconf->val, subconf,
							  srv, &props->balancer); 
			if (ret != ret_ok) 
				return ret;
		} else if (equal_buf_str (&subconf->key, "reuse_max")) {
			props->reuse_max = atoi (subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "in_allow_keepalive")) {
			props->in_allow_keepalive = !! atoi (subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "in_preserve_host")) {
			props->in_preserve_host = !! atoi (subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "out_preserve_server")) {
			props->out_preserve_server = !! atoi (subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "in_header_hide")) {
			cherokee_config_node_foreach (j, subconf) {
				cherokee_avl_add (&props->in_headers_hide, 
						  &CONFIG_NODE(j)->val, NULL);
			}

		} else if (equal_buf_str (&subconf->key, "out_header_hide")) {
			cherokee_config_node_foreach (j, subconf) {
				cherokee_avl_add (&props->out_headers_hide, 
						  &CONFIG_NODE(j)->val, NULL);
			}

		} else if (equal_buf_str (&subconf->key, "in_header_add") ||
			   equal_buf_str (&subconf->key, "out_header_add")) {
			cherokee_config_node_foreach (j, subconf) {
				cherokee_header_add_t *header = NULL;

				ret = header_add_new (&header);
				if (ret != ret_ok)
					return ret_error;

				cherokee_buffer_add_buffer (&header->key, &CONFIG_NODE(j)->key);
				cherokee_buffer_add_buffer (&header->val, &CONFIG_NODE(j)->val);

				if (equal_buf_str (&subconf->key, "in_header_add"))
					cherokee_list_add (&header->listed,
							   &props->in_headers_add);
				else
					cherokee_list_add (&header->listed,
							   &props->out_headers_add);
			}

		} else if (equal_buf_str (&subconf->key, "in_rewrite_request")) {
			ret = cherokee_regex_list_configure (&props->in_request_regexs,
							     subconf, srv->regexs);
			if (ret != ret_ok)
				return ret;

		} else if (equal_buf_str (&subconf->key, "out_rewrite_request")) {
			ret = cherokee_regex_list_configure (&props->out_request_regexs,
							     subconf, srv->regexs);
			if (ret != ret_ok)
				return ret;
		}
	}

	/* Init properties
	 */
	cherokee_handler_proxy_hosts_init (&props->hosts);

	/* Final checks
	 */
	if (props->balancer == NULL) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_HANDLER_NO_BALANCER);
		return ret_error;
	}

	return ret_ok;
}


static int
replace_againt_regex_list (cherokee_buffer_t *in_buf,
			   cherokee_buffer_t *out_buf,
			   cherokee_list_t   *regexs)
{
	int                     re;
	cherokee_list_t        *i;
	cherokee_regex_entry_t *regex_entry;
	cint_t                  ovector[OVECTOR_LEN];

	list_for_each (i, regexs) {
		regex_entry = REGEX_ENTRY(i);
		
		re = pcre_exec (regex_entry->re, NULL, 
				in_buf->buf, in_buf->len, 0, 0,
				ovector, OVECTOR_LEN);
		if (re == 0) {
			LOG_ERROR_S (CHEROKEE_ERROR_HANDLER_REGEX_GROUPS);
		}
		if (re <= 0) {
			continue;
		}

		/* Matched */
		cherokee_regex_substitute (&regex_entry->subs, in_buf, out_buf, ovector, re);
		return 1;
	}

	return 0;
}


static ret_t
add_request (cherokee_handler_proxy_t *hdl,
	     cherokee_buffer_t        *buf)
{
	int                             re;
	ret_t                           ret;
	cherokee_connection_t          *conn  = HANDLER_CONN(hdl);
	cherokee_buffer_t              *tmp   = &HANDLER_THREAD(hdl)->tmp_buf1;
	cherokee_handler_proxy_props_t *props = HDL_PROXY_PROPS(hdl);

	cherokee_buffer_clean (tmp);

	/* Build the URL
	 */
	ret = cherokee_buffer_escape_uri (tmp, &conn->request);
	if (ret != ret_ok) {
		return ret_error;
	}

	if (! cherokee_buffer_is_empty (&conn->pathinfo)) {
		ret = cherokee_buffer_escape_uri (tmp, &conn->pathinfo);
		if (ret != ret_ok) {
			return ret_error;
		}
	}

	if (! cherokee_buffer_is_empty (&conn->query_string)) {
		cherokee_buffer_add_char (tmp, '?');

		ret = cherokee_buffer_add_buffer (tmp, &conn->query_string);
		if (ret != ret_ok) {
			return ret_error;
		}
	}

	TRACE(ENTRIES, "Client request: '%s'\n", tmp->buf);

	/* Check the regexs
	 */
	re = replace_againt_regex_list (tmp, buf, &props->in_request_regexs);
	if (re == 0) {
		/* Did not match any regex, use the raw URL
		 */
		cherokee_buffer_add_buffer (buf, tmp);
	}

	return ret_ok;
}

static void
add_header (cherokee_buffer_t *buf,
	    cherokee_buffer_t *key,
	    cherokee_buffer_t *val)
{
	char *p;
	char *end;

	/* Remove the old header, if existed.
	 */
	p = buf->buf;
	do {
		p = strstr (p, key->buf);
		if (p == NULL) 
			break;

		if (p[key->len] != ':') {
			p += key->len;
			continue;
		}

		end = strchr (p + key->len, CHR_CR);
		if (end) {
			if (end[1] == CHR_LF)
				end += 2;
			else
				end += 1;
		} else {
			end = strchr (p + key->len, CHR_LF);
		}
		if (unlikely (end == NULL))
			return;
		
		cherokee_buffer_remove_chunk (buf, p-buf->buf, end-p);
		break;
	} while (p);

	/* Add the new header
	 */
	cherokee_buffer_add_buffer (buf, key);
	cherokee_buffer_add_str    (buf, ": ");
	cherokee_buffer_add_buffer (buf, val);
	cherokee_buffer_add_str    (buf, CRLF);
}


static ret_t
build_request (cherokee_handler_proxy_t *hdl,
	       cherokee_buffer_t        *buf)
{
	ret_t                           ret;
	cuint_t                         len;
	const char                     *str;
	char                           *begin;
	char                           *colon;
	char                           *end;
	cuint_t                         header_len;
	char                           *header_end;
	cherokee_list_t                *i;
	char                           *ptr;
	cuint_t                         ptr_len;
	char                           *XFF          = NULL;
	cuint_t                         XFF_len      = 0;
	cherokee_boolean_t              XFH          = false;
	cherokee_boolean_t              is_keepalive = false;
	cherokee_boolean_t              is_close     = false;
	cherokee_boolean_t              x_real_ip    = false;
	cherokee_connection_t          *conn         = HANDLER_CONN(hdl);
	cherokee_handler_proxy_props_t *props        = HDL_PROXY_PROPS(hdl);
	cherokee_buffer_t              *tmp          = &HANDLER_THREAD(hdl)->tmp_buf1;

	/* Method */
	ret = cherokee_http_method_to_string (conn->header.method, &str, &len);
	if (ret != ret_ok)
		goto error;

	cherokee_buffer_add (buf, str, len);
	cherokee_buffer_add_char (buf, ' ');

	/* The request */
	ret = add_request (hdl, buf);
	if (ret != ret_ok)
		goto error;

	/* HTTP version */
	ret = cherokee_http_version_to_string (conn->header.version, &str, &len);
	if (ret != ret_ok)
		goto error;

	cherokee_buffer_add_char (buf, ' ');
	cherokee_buffer_add      (buf, str, len);
	cherokee_buffer_add_str  (buf, CRLF);

	/* Add header: "Host: " */
	cherokee_buffer_add_str (buf, "Host: ");
	
	if ((props->in_preserve_host) &&
	    (! cherokee_buffer_is_empty (&conn->host)))
	{
		cherokee_buffer_add_buffer (buf, &conn->host);
	}
	else {
		cherokee_buffer_add_buffer (buf, &hdl->src_ref->host);
		if (hdl->src_ref->port != 80) { 
			cherokee_buffer_add_char    (buf, ':'); 
			cherokee_buffer_add_ulong10 (buf, hdl->src_ref->port); 
		} 
	}
	cherokee_buffer_add_str (buf, CRLF);

	/* Add header: "Connection: " */
	ret = cherokee_header_get_known (&conn->header, header_connection, &ptr, &ptr_len);
	if (ret == ret_ok) {
		if (! strncasecmp (ptr, "Keep-Alive", 10)) {
			is_keepalive = true;
		} else if (! strncasecmp (ptr, "Close", 5)){
			is_close = true;
		}
	}

	if ((props->in_allow_keepalive) &&
	    ((is_keepalive) ||
	     ((conn->header.version == http_version_11) && (! is_close))))
	{
		cherokee_buffer_add_str (buf, "Connection: Keep-Alive" CRLF);
		hdl->pconn->keepalive_in = true;
		
		ret = cherokee_header_get_known (&conn->header, header_keepalive, &ptr, &ptr_len);
		if (ret == ret_ok) {
			cherokee_buffer_add_str (buf, "Keep-Alive: ");
			cherokee_buffer_add     (buf, ptr, ptr_len);
			cherokee_buffer_add_str (buf, CRLF);
		}
	} else {
		cherokee_buffer_add_str (buf, "Connection: Close" CRLF);
		hdl->pconn->keepalive_in = false;
	}

	/* Headers
	 */
	str = strchr (conn->incoming_header.buf, CHR_CR);
	if (str == NULL)
		goto error;

	while ((*str == CHR_CR) || (*str == CHR_LF))
		str++;
	
	/* Add the client headers  */
	cherokee_header_get_length (&conn->header, &header_len);

	begin      = (char *)str;
	header_end = conn->incoming_header.buf + (header_len - 2);

	while (begin < header_end) {
		char chr_end;

		/* Where the line ends */
		end = cherokee_header_get_next_line (begin);
		if (end == NULL)
			break;

		chr_end = *end;
		*end = '\0';

		/* Check the header entry  */
		if ((! strncasecmp (begin, "Host:", 5)) ||
		    (! strncasecmp (begin, "Expect:", 7)) ||
		    (! strncasecmp (begin, "Connection:", 11)) ||
		    (! strncasecmp (begin, "Keep-Alive:", 11)) ||
		    (! strncasecmp (begin, "Transfer-Encoding:", 18)))
		{
			goto next;
		} 
		else if (! strncasecmp (begin, "X-Forwarded-For:", 16))
		{
			XFF = begin + 16;
			while ((*XFF == ' ') && (XFF < end))
				XFF++;

			XFF_len = end - XFF;
			goto next;
		}
		else if (! strncasecmp (begin, "X-Forwarded-Host:", 17))
		{
			XFH = true;
		}
		else if (! strncasecmp (begin, "X-Real-IP:", 10))
		{
			x_real_ip = true;
		}
		else {
			colon = strchr (begin, ':');
			if (unlikely (colon == NULL)) {
				return ret_error;
			}

			*colon = '\0';
			ret = cherokee_avl_get_ptr (&props->in_headers_hide, begin, NULL);
			*colon = ':';
			
			if (ret == ret_ok)
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
	

	/* X-Forwarded-For */
	cherokee_buffer_ensure_size (tmp, CHE_INET_ADDRSTRLEN+1);
	cherokee_socket_ntop (&conn->socket, tmp->buf, tmp->size-1);

	cherokee_buffer_add_str (buf, "X-Forwarded-For: ");
	if (XFF != NULL) {
		cherokee_buffer_add     (buf, XFF, XFF_len);
		cherokee_buffer_add_str (buf, ", ");
	}
	cherokee_buffer_add     (buf, tmp->buf, strlen(tmp->buf));
	cherokee_buffer_add_str (buf, CRLF);

	/* X-Real-IP */
	if (! x_real_ip) {
		cherokee_buffer_add_str (buf, "X-Real-IP: ");
		cherokee_buffer_add     (buf, tmp->buf, strlen(tmp->buf));
		cherokee_buffer_add_str (buf, CRLF);
	}

	/* X-Forwarded-Host */
	if ((XFH == false) && 
	    (! cherokee_buffer_is_empty (&conn->host)))
	{
		cherokee_buffer_add_str    (buf, "X-Forwarded-Host: ");
		cherokee_buffer_add_buffer (buf, &conn->host);

		if (((! conn->socket.is_tls) && (conn->bind->port != 80)) ||
		    ((  conn->socket.is_tls) && (conn->bind->port != 443)))
		{
			cherokee_buffer_add_char    (buf, ':');
			cherokee_buffer_add_ulong10 (buf, conn->bind->port);
		}

		cherokee_buffer_add_str    (buf, CRLF);
	}

	/* X-Forwarded-SSL */
	cherokee_buffer_add_str (buf, "X-Forwarded-SSL: ");
	if(conn->socket.is_tls) {
		cherokee_buffer_add_str (buf, "on");
	} else {
		cherokee_buffer_add_str (buf, "off");
	}
	cherokee_buffer_add_str (buf, CRLF);

	/* Additional headers */
	list_for_each (i, &props->in_headers_add) {
		add_header (buf, &HEADER_ADD(i)->key, &HEADER_ADD(i)->val);
	}

	/* End of Header */
	cherokee_buffer_add_str (buf, CRLF);
	return ret_ok;
error:
	TRACE (ENTRIES",post", "Couldn't build request %s", "\n");
	cherokee_buffer_clean (buf);
	return ret_error;
}


static ret_t
do_connect (cherokee_handler_proxy_t *hdl)
{
	ret_t ret;

	ret = cherokee_socket_connect (&hdl->pconn->socket);
	switch (ret) {
	case ret_ok:
		break;
	case ret_deny:
	case ret_error:
		return ret;
	case ret_eagain:
		ret = cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl),
							   HANDLER_CONN(hdl), 
							   hdl->pconn->socket.socket,
							   FDPOLL_MODE_WRITE, false);
		if (ret != ret_ok) {
			return ret_deny;
		}
		return ret_eagain;
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	return ret_ok;
}

static ret_t
send_post (cherokee_handler_proxy_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	ret = cherokee_post_walk_to_socket (&conn->post, &hdl->pconn->socket);

	TRACE (ENTRIES",post", "Sending POST fd=%d, ret=%d\n", 
	       hdl->pconn->socket.socket, ret);

	switch (ret) {
	case ret_ok:
		TRACE (ENTRIES",post", "%s\n", "finished");
		return ret_ok;

	case ret_eof:
	case ret_error:
		return ret;

	case ret_eagain:
		ret = cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl),
							   conn, hdl->pconn->socket.socket,
							   FDPOLL_MODE_WRITE, false);
		if (ret != ret_ok) {
			return ret_eof;
		}
		return ret_eagain;
		
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}	
}


ret_t
cherokee_handler_proxy_init (cherokee_handler_proxy_t *hdl)
{
	ret_t                           ret;
	cherokee_handler_proxy_poll_t  *poll;
	cherokee_connection_t          *conn  = HANDLER_CONN(hdl);
	cherokee_handler_proxy_props_t *props = HDL_PROXY_PROPS(hdl);


	switch (hdl->init_phase) {
	case proxy_init_start:
		/* Send the Post if needed
		 */
		if (! cherokee_post_is_empty (&conn->post)) {
			cherokee_post_walk_reset (&conn->post);
		}
	
		hdl->init_phase = proxy_init_get_conn;
		TRACE(ENTRIES, "Entering init '%s'\n", "get conn");

	case proxy_init_get_conn:
		/* Check with the load balancer
		 */
		if (hdl->src_ref == NULL) {
			ret = cherokee_balancer_dispatch (props->balancer, conn, &hdl->src_ref);
			if (ret != ret_ok) {
				BIT_UNSET (HANDLER(hdl)->support, hsupport_error);
				conn->error_code = http_service_unavailable;
				return ret_error;
			}

			/* Sanity check */
			if (unlikely (hdl->src_ref->port == -1)) {
				hdl->src_ref->port = 80;
			}
		}
	
		/* Get the connection poll
		 */	
		ret = cherokee_handler_proxy_hosts_get (&props->hosts, hdl->src_ref,
							&poll, props->reuse_max);
		if (unlikely (ret != ret_ok)) {
			conn->error_code = http_service_unavailable;
			return ret_error;
		}

		/* Get a connection
		 */
		ret = cherokee_handler_proxy_poll_get (poll, &hdl->pconn, hdl->src_ref);
		if (unlikely (ret != ret_ok)) {
			conn->error_code = http_service_unavailable;
			return ret_error;
		}
		
	reconnect:
		hdl->init_phase = proxy_init_preconnect;
		TRACE(ENTRIES, "Entering phase '%s'\n", "preconnect");
	
	case proxy_init_preconnect:
		/* Configure if respined
		 */
		if (hdl->respined) {
			cherokee_socket_clean (&hdl->pconn->socket);
			cherokee_socket_close (&hdl->pconn->socket);
		}

		if (! cherokee_socket_configured (&hdl->pconn->socket))
		{
			ret = cherokee_proxy_util_init_socket (&hdl->pconn->socket, hdl->src_ref);
			if (ret != ret_ok) {
				hdl->pconn->keepalive_in = false;
				conn->error_code = http_bad_gateway;
				return ret_error;
			}
		}
		
		hdl->init_phase = proxy_init_connect;
		TRACE(ENTRIES, "Entering phase 'connect': pconn=%p\n", hdl->pconn);

	case proxy_init_connect:
		/* Connect to the target host
		 */
		if (! cherokee_socket_is_connected (&hdl->pconn->socket)) {
			ret = do_connect (hdl);
			switch (ret) {
			case ret_ok:
				break;
			case ret_error:
				conn->error_code = http_bad_gateway;
				hdl->pconn->keepalive_in = false;
				return ret_error;
			case ret_eagain:
				return ret_eagain;
			case ret_deny:
				if (hdl->respined) {
					cherokee_balancer_report_fail (props->balancer, conn, hdl->src_ref);
					conn->error_code = http_bad_gateway;
					hdl->pconn->keepalive_in = false;
					return ret_error;
				}

				if (hdl->src_ref->type == source_interpreter) {
					ret = cherokee_source_interpreter_spawn (SOURCE_INT(hdl->src_ref),
										 CONN_VSRV(conn)->logger);
					switch (ret) {
					case ret_ok:
						break;
					case ret_eagain:
						return ret_eagain;
					default:
						return ret_error;
					}
				}

				hdl->respined = true;
				goto reconnect;
			default:
				hdl->pconn->keepalive_in = false;
				conn->error_code = http_bad_gateway;
				RET_UNKNOWN(ret);
				return ret_error;
			}
		}

		hdl->init_phase = proxy_init_build_headers;
		TRACE(ENTRIES, "Entering phase '%s'\n", "build headers");

	case proxy_init_build_headers:
		/* Build request
		 */
		if (cherokee_buffer_is_empty (&hdl->request)) {
			ret = build_request (hdl, &hdl->request);
			if (unlikely (ret != ret_ok)) {
				conn->error_code = http_bad_gateway;
				hdl->pconn->keepalive_in = false;
				return ret_error;
			}
		}

		hdl->init_phase = proxy_init_send_headers;
		TRACE(ENTRIES, "Entering phase 'send headers'\n%s", hdl->request.buf);

	case proxy_init_send_headers:
		/* Send the request
		 */
		ret = cherokee_handler_proxy_conn_send (hdl->pconn, &hdl->request);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			return ret_eagain;
		case ret_eof:
		case ret_error:
			if (hdl->respined) {
				cherokee_balancer_report_fail (props->balancer, conn, hdl->src_ref);
				conn->error_code = http_bad_gateway;
				hdl->pconn->keepalive_in = false;
				return ret_error;
			}

			hdl->respined = true;
			goto reconnect;
		default:
			hdl->pconn->keepalive_in = false;
			conn->error_code = http_bad_gateway;
			RET_UNKNOWN(ret);
			return ret_error;
		}

		hdl->init_phase = proxy_init_send_post;
		TRACE(ENTRIES, "Entering phase '%s'\n", "send post");

	case proxy_init_send_post:
		if (! cherokee_post_is_empty (&conn->post)) {
			ret = send_post (hdl);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eagain:
				return ret_eagain;
			case ret_error:
				hdl->pconn->keepalive_in = false;
				conn->error_code = http_bad_gateway;
				return ret_error;
			default:
				hdl->pconn->keepalive_in = false;
				conn->error_code = http_bad_gateway;
				RET_UNKNOWN(ret);
				return ret_error;
			}
		}

		hdl->init_phase = proxy_init_read_header;
		TRACE(ENTRIES, "Entering phase '%s'\n", "read header");

	case proxy_init_read_header:
		/* Read the client header
		 */
		ret = cherokee_handler_proxy_conn_recv_headers (hdl->pconn, &hdl->tmp);
		switch (ret) {
		case ret_ok:
			/* Turn Error Handling on. From this point the
			 * handler object supports error handling.
			 */
			HANDLER(hdl)->support |= hsupport_error;
			
			/* Got the header.
			 */
			break;
		case ret_eagain:
			ret = cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl),
								   HANDLER_CONN(hdl), 
								   hdl->pconn->socket.socket,
								   FDPOLL_MODE_READ, false);
			if (ret != ret_ok) {
				hdl->pconn->keepalive_in = false;
				conn->error_code = http_bad_gateway;
				return ret_error;
			}
			return ret_eagain;
		case ret_eof:
		case ret_error:
			/* The socket isn't really connected
			 */
			if (hdl->respined) {
				cherokee_balancer_report_fail (props->balancer, conn, hdl->src_ref);
				hdl->pconn->keepalive_in = false;
				conn->error_code = http_bad_gateway;
				return ret_error;
			}

			cherokee_post_walk_reset (&conn->post);
			hdl->respined = true;
			goto reconnect;
		default:
			hdl->pconn->keepalive_in = false;
			conn->error_code = http_bad_gateway;
			RET_UNKNOWN(ret);
			return ret_error;
		}
		break;		

	default:
		SHOULDNT_HAPPEN;
	}

	TRACE(ENTRIES, "Exiting %s\n", "init");
	return ret_ok;
}


static ret_t
parse_server_header (cherokee_handler_proxy_t *hdl,
		     cherokee_buffer_t        *buf_in,
		     cherokee_buffer_t        *buf_out)
{
	int                             re;
	ret_t                           ret;
	char                           *p;
	char                           *begin;
	char                           *end;
	char                           *colon;
	char                           *header_end;
	cherokee_list_t                *i;
	cherokee_http_version_t         version;
	cherokee_connection_t          *conn         = HANDLER_CONN(hdl);
	cherokee_handler_proxy_props_t *props        = HDL_PROXY_PROPS(hdl);

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
		version                  = http_version_10;
		hdl->pconn->keepalive_in = false;

	} else if (strncmp (p, "0.9", 3) == 0) {
		version                  = http_version_09;
		hdl->pconn->keepalive_in = false;
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

	/* Skip 100 Continue headers - pseudo responses to the
	 * "Expect: 100-Continue" client header.
	 */
	if (conn->error_code == http_continue) {
		cherokee_buffer_move_to_begin (buf_in, header_end - buf_in->buf);
		return ret_eagain;
	}

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
		if (strncasecmp (begin, "Transfer-Encoding:", 18) == 0) {
			char *c = begin + 18;
			while (*c == ' ') c++;

			if (strncasecmp (c, "chunked", 7) == 0) {
				hdl->pconn->enc = pconn_enc_chunked;
			}

			goto next;

		} else  if (strncasecmp (begin, "Connection:", 11) == 0) {
			char *c = begin + 11;
			while (*c == ' ') c++;

			if (strncasecmp (c, "Keep-Alive", 7) == 0) {
				hdl->pconn->keepalive_in = true;
			} else {
				hdl->pconn->keepalive_in = false;
			}

			goto next;

		} else  if (strncasecmp (begin, "Keep-Alive:", 11) == 0) {
			goto next;

		} else  if (strncasecmp (begin, "Content-Length:", 15) == 0) {
			char *c = begin + 15;
			while (*c == ' ') c++;

			hdl->pconn->enc     = pconn_enc_known_size;
			hdl->pconn->size_in = strtoll (c, NULL, 10);

			if (! cherokee_connection_should_include_length(conn)) {
				goto next;
			}

			HANDLER(hdl)->support |= hsupport_length;

		} else if ((! props->out_preserve_server) &&
			   (strncasecmp (begin, "Server:", 7) == 0)) {

			cherokee_buffer_add_str (buf_out, "Server: ");
			cherokee_buffer_add_buffer (buf_out, &CONN_BIND(conn)->server_string);
			cherokee_buffer_add_str (buf_out, CRLF);
			goto next;

		} else  if (strncasecmp (begin, "Location:", 9) == 0) {
			cherokee_buffer_t *tmp1 = &HANDLER_THREAD(hdl)->tmp_buf1;
			cherokee_buffer_t *tmp2 = &HANDLER_THREAD(hdl)->tmp_buf2;
	
			cherokee_buffer_clean (tmp2);
			cherokee_buffer_clean (tmp1);
			cherokee_buffer_add   (tmp1, begin+10, end-(begin+10));

			re = replace_againt_regex_list (tmp1, tmp2, &props->out_request_regexs);
			if (re) {
				cherokee_buffer_add_str    (buf_out, "Location: ");
				cherokee_buffer_add_buffer (buf_out, tmp2);
				cherokee_buffer_add_str    (buf_out, CRLF);
				goto next;
			}

		} else if (strncasecmp (begin, "Content-Encoding:", 17) == 0) {
			BIT_SET (conn->options, conn_op_cant_encoder);

		} else {
			colon = strchr (begin, ':');
			if (unlikely (colon == NULL)) {
				return ret_error;
			}

			*colon = '\0';
			ret = cherokee_avl_get_ptr (&props->out_headers_hide, begin, NULL);
			*colon = ':';

			if (ret == ret_ok)
				goto next;
		}

		cherokee_buffer_add     (buf_out, begin, end-begin);
		cherokee_buffer_add_str (buf_out, CRLF);
		
	next:
		/* Prepare next iteration */
		*end = chr_end;
		while ((*end == CHR_CR) || (*end == CHR_LF))
			end++;
		begin = end;		
	}	

	/* Additional headers */
	list_for_each (i, &props->out_headers_add) {
		add_header (buf_out, &HEADER_ADD(i)->key, &HEADER_ADD(i)->val);
	}

	/* Deal with Content-Encoding: If the response is no encoded,
	 * and the handler is configured to encode, it has to add the
	 * encoder headers at this point.
	 */
	if (conn->encoder_new_func) {
		ret = cherokee_connection_instance_encoder (conn);
		if (ret == ret_ok) {
			cherokee_encoder_add_headers (conn->encoder, buf_out);
		}
	}

	/* Special case: Client uses Keepalive, the back-end sent a
	 * no-body reply with no Content-Length.
	 */
	if ((conn->keepalive) &&
	    (hdl->pconn->enc != pconn_enc_known_size) &&
	    (! http_code_with_body (HANDLER_CONN(hdl)->error_code)))
	{
		cherokee_buffer_add_str (buf_out, "Content-Length: 0"CRLF);
	}

	TRACE(ENTRIES, " IN - Header:\n%s",     buf_in->buf);
	TRACE(ENTRIES, " IN - Keepalive: %d\n", hdl->pconn->keepalive_in);
	TRACE(ENTRIES, " IN - Encoding: %s\n", (hdl->pconn->enc == pconn_enc_chunked) ? "chunked":"plain");
	TRACE(ENTRIES, " IN - Size: %llu\n",   (hdl->pconn->enc == pconn_enc_known_size) ? hdl->pconn->size_in : (off_t)0);
	TRACE(ENTRIES, "OUT - Header:\n%s",     buf_out->buf);

	return ret_ok;

error:
	conn->error_code = http_version_not_supported;
	return ret_error;
}


ret_t
cherokee_handler_proxy_add_headers (cherokee_handler_proxy_t *hdl,
				    cherokee_buffer_t        *buf)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	if (unlikely (hdl->pconn == NULL)) {
		return ret_error;
	}

	/* Parse the incoming header
	 */
	ret = parse_server_header (hdl, &hdl->pconn->header_in_raw, buf);
	switch (ret) {
	case ret_ok:
		break;
	case ret_eagain:
		hdl->init_phase = proxy_init_read_header;
		conn->phase     = phase_init;
		return ret_eagain;
	default:
		return ret_error;
	}

	/* If the reply has no body, let's mark it
	 */
	if (! http_code_with_body (HANDLER_CONN(hdl)->error_code)) {
		hdl->got_all = true;

		TRACE(ENTRIES, "Reply is %d, it has no body. Marking as 'got all'.\n",
		      HANDLER_CONN(hdl)->error_code);
	}

	return ret_ok;
}


static ret_t
check_chunked (cherokee_handler_proxy_t *hdl,
	       char                     *begin,
	       cuint_t                   len,
	       size_t                   *head,
	       ssize_t                  *size)
{
	char *p = begin;

	UNUSED(hdl);

	/* Iterate through the number */
	while (((*p >= '0') && (*p <= '9')) ||
	       ((*p >= 'a') && (*p <= 'f')) ||
	       ((*p >= 'A') && (*p <= 'F')))
		p++;

	/* Check the CRLF after the length */
	if (p[0] != CHR_CR)
		return ret_error;
	if (p[1] != CHR_LF)
		return ret_error;

	/* Read the length */
	*size = strtoul (begin, &p, 16);
	p += 2;
	*head = p - begin;

	/* Is it complete? */
	if (*size == 0) {
		return ret_eof;
	}

	if (len < (*head + *size + 2)) {
		*head = 0;
		*size = 0;
		return ret_eagain;
	}

	/* Check the CRLF at the end 
	*/
	if ((p[(*size)]     != CHR_CR) ||
	    (p[(*size) + 1] != CHR_LF))
	{
		*head = 0;
		*size = 0;
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_handler_proxy_step (cherokee_handler_proxy_t *hdl,
			     cherokee_buffer_t        *buf)
{
	ret_t  ret;
	size_t size = 0;

	/* No-encoding: known size
	 */
	switch (hdl->pconn->enc) {
	case pconn_enc_none:
	case pconn_enc_known_size:
		/* Remaining from the header
		 */
		if (! cherokee_buffer_is_empty (&hdl->tmp)) {
			hdl->pconn->sent_out += hdl->tmp.len;

			cherokee_buffer_add_buffer (buf, &hdl->tmp);			
			cherokee_buffer_clean (&hdl->tmp);
		
			if ((hdl->pconn->enc == pconn_enc_known_size) &&
			    (hdl->pconn->sent_out >= hdl->pconn->size_in))
			{
				hdl->got_all = true;
				return ret_eof_have_data;
			}

			return ret_ok;
		}

		/* Might have already finished (Content-length: 0)
		 */
		if ((hdl->pconn->enc == pconn_enc_known_size) &&
		    (hdl->pconn->sent_out >= hdl->pconn->size_in))
		{
			hdl->got_all = true;
			return ret_eof;
		}

		/* Read
		 */
		ret = cherokee_socket_bufread (&hdl->pconn->socket, buf,
					       DEFAULT_BUF_SIZE, &size);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eof:
		case ret_error:
			hdl->pconn->keepalive_in = false;
			return ret;
		case ret_eagain:
			cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl),
							     HANDLER_CONN(hdl), 
							     hdl->pconn->socket.socket,
							     FDPOLL_MODE_READ, false);
			return ret_eagain;
		default:
			RET_UNKNOWN(ret);
			return ret_error;
		}
		
		if (size > 0) {
			hdl->pconn->sent_out += size;

			if ((hdl->pconn->enc == pconn_enc_known_size) &&
			    (hdl->pconn->sent_out >= hdl->pconn->size_in))
			{
				hdl->got_all = true;
				return ret_eof_have_data;
			}

			return ret_ok;
		}

		return ret_eagain;

	case pconn_enc_chunked: {
		/* Chunked encoded reply
		 */
		char    *p;
		char    *end;
		ret_t    ret_read;
		size_t   head_size;
		ssize_t  body_size;
		size_t   copied       = 0;
		size_t   copied_total = 0;

		/* Read a little bit more
		 */
		ret_read = cherokee_socket_bufread (&hdl->pconn->socket, &hdl->tmp,
						    DEFAULT_BUF_SIZE, &size);

		/* Process the chunked encoding
		 */
		p   = hdl->tmp.buf;
		end = hdl->tmp.buf + hdl->tmp.len;

		while (true) {			
			/* Is it within bounds? */
			if (p + (sizeof("0"CRLF_CRLF)-1) > end) {
				ret = ret_eagain;
				goto out;
			}

			/* Read next chunk */
			head_size =  0;
			body_size = -1;

			ret = check_chunked (hdl, p, (end - p), &head_size, &body_size);
			switch (ret) {
			case ret_ok:
			case ret_eof:
				break;
			case ret_error:
			case ret_eagain:
				goto out;
			default:
				RET_UNKNOWN(ret);
				return ret_error;
			}
			
			/* There is a chunk to send */
			if (body_size > 0) {
				cherokee_buffer_add (buf, (p+head_size), body_size);
			}

			copied = (head_size + body_size + 2);
			
			copied_total += copied;
			p            += copied;

			if (ret == ret_eof)
				break;
		}

	out:
		if (copied_total > 0) {
			cherokee_buffer_move_to_begin (&hdl->tmp, copied_total);
		}

		if (buf->len > 0) {
			if (ret == ret_eof) {
				hdl->got_all = true;
				return ret_eof_have_data;
			}
			return ret_ok;
		}

		if ((ret == ret_eof) ||
		    (ret_read == ret_eof))
		{
			hdl->pconn->keepalive_in = false;
			return ret_eof;
		}

		if (ret_read == ret_eagain) {
 			cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl), 
 							     HANDLER_CONN(hdl), 
 							     hdl->pconn->socket.socket, 
 							     FDPOLL_MODE_READ, false); 
			return ret_eagain;
		}

		return ret;
	} /* chunked */

	default:
		SHOULDNT_HAPPEN;
	}
	
	return ret_error;
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

	HANDLER(n)->support = hsupport_full_headers;

	/* Init
	 */
	n->pconn      = NULL;
	n->src_ref    = NULL;
	n->init_phase = proxy_init_start;
	n->respined   = false;
	n->got_all    = false;

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

	if (hdl->pconn != NULL) {
		if (! hdl->got_all) {
			hdl->pconn->keepalive_in = false;
			TRACE (ENTRIES, "Did not get all, turning keepalive %s\n", "off");
		} 

		cherokee_handler_proxy_conn_release (hdl->pconn);
	}

	return ret_ok;
}

