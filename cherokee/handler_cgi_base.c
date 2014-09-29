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
#include "handler_cgi_base.h"

#include "socket.h"
#include "util.h"

#include "connection-protected.h"
#include "server-protected.h"
#include "header-protected.h"
#include "handler_file.h"

#define ENTRIES "cgibase"

#define set_env(cgi,key,val,len) \
	set_env_pair (cgi, key, sizeof(key)-1, val, len)

static cherokee_handler_file_props_t handler_file_props;

ret_t
cherokee_handler_cgi_base_init (cherokee_handler_cgi_base_t              *cgi,
                                cherokee_connection_t                    *conn,
                                cherokee_plugin_info_handler_t           *info,
                                cherokee_handler_props_t                 *props,
                                cherokee_handler_cgi_base_add_env_pair_t  add_env_pair,
                                cherokee_handler_cgi_base_read_from_cgi_t read_from_cgi)
{
	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(cgi), conn, props, info);

	/* Init to default values
	 */
	cgi->init_phase          = hcgi_phase_build_headers;
	cgi->content_length      = 0;
	cgi->got_eof             = false;
	cgi->file_handler        = NULL;

	cherokee_buffer_init (&cgi->xsendfile);
	cherokee_buffer_init (&cgi->executable);

	cherokee_buffer_init (&cgi->data);
	cherokee_buffer_ensure_size (&cgi->data, 2*1024);

	/* Virtual methods
	 */
	cgi->add_env_pair  = add_env_pair;
	cgi->read_from_cgi = read_from_cgi;

	/* Read the properties
	 */
	if (PROP_CGI_BASE(props)->allow_xsendfile == false) {
		HANDLER(cgi)->support = hsupport_nothing;
	} else {
		HANDLER(cgi)->support = hsupport_range;
	}

	return ret_ok;
}


typedef struct {
	cherokee_list_t   entry;
	cherokee_buffer_t env;
	cherokee_buffer_t val;
} env_item_t;


static env_item_t *
env_item_new (cherokee_buffer_t *key, cherokee_buffer_t *val)
{
	env_item_t *n;

	n = malloc (sizeof (env_item_t));
	if (unlikely (n == NULL)) {
		return NULL;
	}

	INIT_LIST_HEAD (&n->entry);
	cherokee_buffer_init (&n->env);
	cherokee_buffer_init (&n->val);

	cherokee_buffer_add_buffer(&n->env, key);
	cherokee_buffer_add_buffer(&n->val, val);

	return n;
}

static void
env_item_free (void *p)
{
	env_item_t *n = (env_item_t *)p;

	cherokee_buffer_mrproper (&n->env);
	cherokee_buffer_mrproper (&n->val);
	free (p);
}

ret_t
cherokee_handler_cgi_base_props_init_base (cherokee_handler_cgi_base_props_t *props, module_func_props_free_t free_func)
{
	return cherokee_handler_props_init_base (HANDLER_PROPS(props), free_func);
}

ret_t
cherokee_handler_cgi_base_props_free (cherokee_handler_cgi_base_props_t *props)
{
	cherokee_list_t *i, *tmp;

	cherokee_buffer_mrproper (&props->script_alias);
	cherokee_x_real_ip_mrproper (&props->x_real_ip);

	list_for_each_safe (i, tmp, &props->system_env) {
		env_item_free (i);
	}

	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}

ret_t
cherokee_handler_cgi_base_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                              ret;
	cherokee_list_t                   *i, *j;
	cherokee_handler_cgi_base_props_t *props;

	UNUSED(srv);

	/* Sanity check: This class is pure virtual,
	 * it shouldn't allocate memory here.
	 * Check that the object space has been already instanced by its father.
	 */
	if (*_props == NULL) {
		SHOULDNT_HAPPEN;
		return ret_ok;
	}

	/* Init
	 */
	props = PROP_CGI_BASE(*_props);

	INIT_LIST_HEAD (&props->system_env);
	cherokee_buffer_init (&props->script_alias);
	cherokee_x_real_ip_init (&props->x_real_ip);

	props->is_error_handler = true;
	props->change_user      = false;
	props->check_file       = true;
	props->allow_xsendfile  = false;
	props->pass_req_headers = true;
	props->use_cache        = true;

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "script_alias")) {
			ret = cherokee_buffer_add_buffer (&props->script_alias, &subconf->val);
			if (ret != ret_ok)
				return ret;

		} else if (equal_buf_str (&subconf->key, "env")) {
			cherokee_config_node_foreach (j, subconf) {
				env_item_t             *env;
				cherokee_config_node_t *subconf2 = CONFIG_NODE(j);

				env = env_item_new (&subconf2->key, &subconf2->val);
				if (env == NULL)
					return ret_error;

				cherokee_list_add_tail (LIST(env), &props->system_env);
			}
		} else if (equal_buf_str (&subconf->key, "error_handler")) {
			ret = cherokee_atob (subconf->val.buf, &props->is_error_handler);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "change_user")) {
			ret = cherokee_atob (subconf->val.buf, &props->change_user);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "check_file")) {
			ret = cherokee_atob (subconf->val.buf, &props->check_file);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "xsendfile")) {
			ret = cherokee_atob (subconf->val.buf, &props->allow_xsendfile);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "pass_req_headers")) {
			ret = cherokee_atob (subconf->val.buf, &props->pass_req_headers);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "iocache")) {
			ret = cherokee_atob (subconf->val.buf, &props->use_cache);
			if (ret != ret_ok) return ret;
		}
	}

	ret = cherokee_x_real_ip_configure (&props->x_real_ip, conf);
	if (ret != ret_ok) {
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_handler_cgi_base_free (cherokee_handler_cgi_base_t *cgi)
{
	if (cgi->file_handler) {
		cherokee_handler_free (cgi->file_handler);
	}

	cherokee_buffer_mrproper (&cgi->data);
	cherokee_buffer_mrproper (&cgi->executable);
	cherokee_buffer_mrproper (&cgi->xsendfile);

	return ret_ok;
}


ret_t
cherokee_handler_cgi_base_build_basic_env (
        cherokee_handler_cgi_base_t              *cgi,
        cherokee_handler_cgi_base_add_env_pair_t  set_env_pair,
        cherokee_connection_t                    *conn,
        cherokee_buffer_t                        *tmp)
{
	int                                re;
	ret_t                              ret;
	cherokee_boolean_t                 remote_addr_set;
	char                              *p                = NULL;
	cuint_t                            p_len            = 0;
	cherokee_bind_t                   *bind             = CONN_BIND(HANDLER_CONN(cgi));
	cherokee_handler_cgi_base_props_t *cgi_props        = HANDLER_CGI_BASE_PROPS(cgi);
	static char                       *env_PATH         = NULL;
	static int                         env_PATH_len     = 0;

	char remote_ip[CHE_INET_ADDRSTRLEN+1];
	CHEROKEE_TEMP(temp, 32);

	/* Set the basic variables
	 */
	set_env (cgi, "SERVER_SOFTWARE",
	         bind->server_string.buf,
	         bind->server_string.len);

	set_env (cgi, "SERVER_SIGNATURE",  "<address>Cherokee Web Server</address>", 38);
	set_env (cgi, "GATEWAY_INTERFACE", "CGI/1.1", 7);

	/* $PATH
	 */
	if (unlikely (env_PATH == NULL)) {
		env_PATH = getenv("PATH");
		if (env_PATH == NULL) {
			env_PATH = "/bin:/usr/bin:/sbin:/usr/sbin";
		}

		env_PATH_len = strlen (env_PATH);
	}

	set_env (cgi, "PATH", env_PATH, env_PATH_len);

	/* Document Root:
	 */
	set_env (cgi, "DOCUMENT_ROOT",
	                 conn->local_directory.buf,
	                 conn->local_directory.len);

	/* REMOTE_(ADDR/PORT): X-Real-IP
	 */
	remote_addr_set = false;

	if (cgi_props->x_real_ip.enabled)
	{
		/* The request has a X-REAL-IP entry */
		cherokee_header_get_known (&conn->header, header_x_real_ip, &p, &p_len);

		/* The request has a X-Forwared-For entry */
		if (p == NULL) {
			cherokee_header_get_known (&conn->header, header_x_forwarded_for, &p, &p_len);
		}

		if (p != NULL)
		{
			/* The remote host is allowed to send it */
			ret = cherokee_x_real_ip_is_allowed (&cgi_props->x_real_ip, &conn->socket);
			if (ret == ret_ok) {
				cuint_t     i;
				const char *port_end = NULL;
				const char *colon    = NULL;
				const char *end      = NULL;

				for (i=0; i < p_len; i++) {
					if ((p[i] == ',') && (end == NULL)) {
						end = &p[i];
						while ((end > p) && ((*end == ',')||(*end == ' '))) {
							end--;
						}
						end++;
						break;
					}

					if (p[i] == ':') {
						if (colon != NULL) {
							/* This must be an IPv6 address */
							colon = NULL;
							break;
						}
						colon = &p[i];
					}
				}

				if (end == NULL) {
					end = p + p_len;
				}

				if (colon != NULL) {
					port_end = colon + 1;
					while ((*port_end >= '0') && (*port_end <= '9')) {
						port_end++;
					}

					set_env (cgi, "REMOTE_ADDR", p, colon - p);
					set_env (cgi, "REMOTE_PORT", colon+1, port_end - (colon+1));
				} else {
					set_env (cgi, "REMOTE_ADDR", p, end - p);
				}

				remote_addr_set = true;
			}
		}
	}

	/* REMOTE_(ADDR/PORT): socket origin
	 */
	if (! remote_addr_set) {
		/* REMOTE_ADDR */
		memset (remote_ip, 0, sizeof(remote_ip));
		cherokee_socket_ntop (&conn->socket, remote_ip, sizeof(remote_ip)-1);
		set_env (cgi, "REMOTE_ADDR", remote_ip, strlen(remote_ip));

		/* REMOTE_PORT */
		re = snprintf (temp, temp_size, "%d", SOCKET_SIN_PORT(&conn->socket));
		if (re > 0) {
			set_env (cgi, "REMOTE_PORT", temp, re);
		}
	}

	/* HTTP_HOST and SERVER_NAME. The difference between them is that
	 * HTTP_HOST can include the ":PORT" text, and SERVER_NAME only
	 * the name
	 */
	cherokee_header_copy_known (&conn->header, header_host, tmp);
	if (! cherokee_buffer_is_empty(tmp)) {
		set_env (cgi, "HTTP_HOST", tmp->buf, tmp->len);

		p = strchr (tmp->buf, ':');
		if (p != NULL) {
			set_env (cgi, "SERVER_NAME", tmp->buf, p - tmp->buf);
		} else {
			set_env (cgi, "SERVER_NAME", tmp->buf, tmp->len);
		}
	} else {
		ret = cherokee_gethostname (tmp);
		if (ret == ret_ok) {
			set_env (cgi, "SERVER_NAME", tmp->buf, tmp->len);
		} else {
			LOG_WARNING_S (CHEROKEE_ERROR_HANDLER_CGI_GET_HOSTNAME);
		}
	}

	/* Content-Type
	 */
	cherokee_buffer_clean (tmp);
	ret = cherokee_header_copy_known (&conn->header, header_content_type, tmp);
	if (ret == ret_ok) {
		set_env (cgi, "CONTENT_TYPE", tmp->buf, tmp->len);
	}

	/* Query string
	 */
	if (conn->query_string.len > 0)
		set_env (cgi, "QUERY_STRING", conn->query_string.buf, conn->query_string.len);
	else
		set_env (cgi, "QUERY_STRING", "", 0);

	/* HTTP protocol version
	 */
	ret = cherokee_http_version_to_string (conn->header.version, (const char **) &p, &p_len);
	if (ret >= ret_ok)
		set_env (cgi, "SERVER_PROTOCOL", p, p_len);

	/* Set the method
	 */
	ret = cherokee_http_method_to_string (conn->header.method, (const char **) &p, &p_len);
	if (ret >= ret_ok)
		set_env (cgi, "REQUEST_METHOD", p, p_len);

	/* Remote user
	 */
	if (conn->validator && !cherokee_buffer_is_empty (&conn->validator->user)) {
		/* Only set when user authenticated (bug #467) */
		set_env (cgi, "REMOTE_USER", conn->validator->user.buf, conn->validator->user.len);
	}

	/* Set PATH_INFO
	 */
	if (! cherokee_buffer_is_empty (&conn->pathinfo))
		set_env (cgi, "PATH_INFO", conn->pathinfo.buf, conn->pathinfo.len);
	else
		set_env (cgi, "PATH_INFO", "", 0);

	/* Set REQUEST_URI:
	 *
	 * Use request + query_string unless the connections has been
	 * redirected from common and targets a full path index file.
	 */
	cherokee_buffer_clean (tmp);

	if (conn->options & conn_op_root_index) {
		cherokee_header_copy_request_w_args (&conn->header, tmp);
	}
	else {
		if (! cherokee_buffer_is_empty (&conn->userdir)) {
			cherokee_buffer_add_str    (tmp, "/~");
			cherokee_buffer_add_buffer (tmp, &conn->userdir);
		}

		if (! cherokee_buffer_is_empty (&conn->request_original)) {
			cherokee_buffer_add_buffer (tmp, &conn->request_original);
			if (! cherokee_buffer_is_empty (&conn->query_string_original)) {
				cherokee_buffer_add_char   (tmp, '?');
				cherokee_buffer_add_buffer (tmp, &conn->query_string_original);
			}
		} else {
			cherokee_buffer_add_buffer (tmp, &conn->request);

			if (! cherokee_buffer_is_empty (&conn->query_string)) {
				cherokee_buffer_add_char (tmp, '?');
				cherokee_buffer_add_buffer (tmp, &conn->query_string);
			}
		}
	}
	set_env (cgi, "REQUEST_URI", tmp->buf, tmp->len);

	/* Set SCRIPT_URL
	 */
	if (! cherokee_buffer_is_empty (&conn->userdir)) {
		cherokee_buffer_clean      (tmp);
		cherokee_buffer_add_str    (tmp, "/~");
		cherokee_buffer_add_buffer (tmp, &conn->userdir);
		cherokee_buffer_add_buffer (tmp, &conn->request);
		set_env (cgi, "SCRIPT_URL", tmp->buf, tmp->len);
	} else {
		set_env (cgi, "SCRIPT_URL", conn->request.buf, conn->request.len);
	}

	/* Set HTTPS and SERVER_PORT
	 */
	if (conn->socket.is_tls) {
		set_env (cgi, "HTTPS", "on", 2);
	} else  {
		set_env (cgi, "HTTPS", "off", 3);
	}

	set_env (cgi, "SERVER_PORT",
		 bind->server_port.buf,
		 bind->server_port.len);

	/* Set SERVER_ADDR
	 */
	if (cherokee_buffer_is_empty (&bind->ip)) {
		cherokee_sockaddr_t my_address;
		cuint_t             my_address_len = 0;
		char                ip_str[CHE_INET_ADDRSTRLEN+1];

		my_address_len = sizeof(my_address);
		re = getsockname (SOCKET_FD(&conn->socket),
				  (struct sockaddr *)&my_address,
				  &my_address_len);
		if (re == 0) {
			cherokee_ntop (my_address.sa_in.sin_family,
				       (struct sockaddr *) &my_address,
				       ip_str, sizeof(ip_str)-1);

			set_env (cgi, "SERVER_ADDR",
				 ip_str, strlen(ip_str));
		}
	} else {
		set_env (cgi, "SERVER_ADDR",
			 bind->server_address.buf,
			 bind->server_address.len);
	}

	/* Internal error redirection:
	 * It is okay if the QS is empty.
	 */
	if (! cherokee_buffer_is_empty (&conn->error_internal_url)) {
		set_env (cgi, "REDIRECT_URL",
			 conn->error_internal_url.buf,
			 conn->error_internal_url.len);

		set_env (cgi, "REDIRECT_QUERY_STRING",
			 conn->error_internal_qs.buf,
			 conn->error_internal_qs.len);
	}

	/* Authentication
	 */
	switch (conn->req_auth_type) {
	case http_auth_nothing:
		break;
	case http_auth_basic:
		set_env (cgi, "AUTH_TYPE", "Basic", 5);
		break;
	case http_auth_digest:
		set_env (cgi, "AUTH_TYPE", "Digest", 6);
		break;
	}

	/* HTTP variables
	 */
	ret = cherokee_header_get_known (&conn->header, header_accept, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_ACCEPT", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_accept_charset, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_ACCEPT_CHARSET", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_accept_encoding, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_ACCEPT_ENCODING", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_accept_language, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_ACCEPT_LANGUAGE", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_authorization, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_AUTHORIZATION", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_connection, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_CONNECTION", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_cookie, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_COOKIE", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_if_modified_since, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_IF_MODIFIED_SINCE", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_if_none_match, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_IF_NONE_MATCH", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_if_range, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_IF_RANGE", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_keepalive, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_KEEP_ALIVE", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_range, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_RANGE", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_referer, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_REFERER", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_user_agent, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_USER_AGENT", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_x_forwarded_for, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_X_FORWARDED_FOR", p, p_len);
	}

	ret = cherokee_header_get_known (&conn->header, header_x_forwarded_host, &p, &p_len);
	if (ret == ret_ok) {
		set_env (cgi, "HTTP_X_FORWARDED_HOST", p, p_len);
	}

	/* TODO: Fill the others CGI environment variables
	 *
	 * http://hoohoo.ncsa.uiuc.edu/cgi/env.html
	 * http://cgi-spec.golux.com/cgi-120-00a.html
	 */

	return ret_ok;
}


static ret_t
foreach_header_add_unknown_variable (cherokee_buffer_t *header,
                                     cherokee_buffer_t *content,
                                     void              *data)
{
	cuint_t                      i;
	cherokee_handler_cgi_base_t *cgi = HDL_CGI_BASE(data);

	/* Transfor 'Headers' to HTTP_HEADERS
	 * NOTE: header is a copy, it can be modified
	 */
	for (i=0; i<header->len; i++) {
		if ((header->buf[i] >= 'a') &&
		    (header->buf[i] <= 'z'))
		{
			header->buf[i] -= ('a' - 'A');
		} else if (header->buf[i] == '-') {
			header->buf[i] = '_';
		}
	}

	cherokee_buffer_prepend_str (header, "HTTP_");

	/* Add it to the *CGI environment
	 */
	cgi->add_env_pair (cgi,
	                   header->buf, header->len,
	                   content->buf, content->len);
	return ret_ok;
}


ret_t
cherokee_handler_cgi_base_build_envp (cherokee_handler_cgi_base_t *cgi, cherokee_connection_t *conn)
{
	ret_t                              ret;
	cherokee_list_t                   *i;
	cherokee_buffer_t                 *name;
	cuint_t                            len       = 0;
	const char                        *p         = "";
	cherokee_buffer_t                  tmp       = CHEROKEE_BUF_INIT;
	cherokee_handler_cgi_base_props_t *cgi_props = HANDLER_CGI_BASE_PROPS(cgi);

	/* Add user defined variables at the beginning,
	 * these have precedence..
	 */
	list_for_each (i, &cgi_props->system_env) {
		env_item_t *env = (env_item_t *)i;
		cgi->add_env_pair (cgi,
		                   env->env.buf, env->env.len,
		                   env->val.buf, env->val.len);
	}

	/* Pass request headers.
	 * Usually X-Something..
	 */
	if (cgi_props->pass_req_headers) {
		cherokee_header_foreach_unknown (&conn->header,
		                                 foreach_header_add_unknown_variable, cgi);
	}

	/* Add the basic enviroment variables
	 */
	ret = cherokee_handler_cgi_base_build_basic_env (cgi, cgi->add_env_pair, conn, &tmp);
	if (unlikely (ret != ret_ok)) return ret;

	/* SCRIPT_NAME:
	 */
	if (! cgi_props->check_file) {
		/* SCGI or FastCGI:
		 *
		 * - If the SCGI is handling / it is ''
		 * - Otherwise, it is the web_directory.
		 */
		if (conn->web_directory.len > 1) {
			cgi->add_env_pair (cgi, "SCRIPT_NAME", 11,
			                   conn->web_directory.buf,
			                   conn->web_directory.len);
		} else {
			cgi->add_env_pair (cgi, "SCRIPT_NAME", 11, "", 0);
		}

	} else {
		/* CGI:
		 *
		 * It is the request without the pathinfo if it exists
		 */
		cherokee_buffer_clean (&tmp);
		if (cherokee_buffer_is_empty (&cgi_props->script_alias)) {
			/* cgi */
			name = &cgi->executable;

			if (conn->local_directory.len > 0){
				p = name->buf + conn->local_directory.len;
				len = (name->buf + name->len) - p;
			} else {
				p = name->buf;
				len = name->len;
			}
		}

		if (! cherokee_buffer_is_empty (&conn->userdir)) {
			cherokee_buffer_add_str    (&tmp, "/~");
			cherokee_buffer_add_buffer (&tmp, &conn->userdir);
		}

		if (cherokee_connection_use_webdir(conn)) {
			cherokee_buffer_add_buffer (&tmp, &conn->web_directory);
		}

		if (len > 0)
			cherokee_buffer_add (&tmp, p, len);

		cgi->add_env_pair (cgi, "SCRIPT_NAME", 11, tmp.buf, tmp.len);
	}

	/* Set PATH_TRANSLATED; only if PATH_INFO is set
	 */
	if (! cherokee_buffer_is_empty (&conn->pathinfo)) {
		cherokee_buffer_add_buffer (&conn->local_directory,
		                            &conn->pathinfo);

		cgi->add_env_pair (cgi, "PATH_TRANSLATED", 15,
		                   conn->local_directory.buf,
		                   conn->local_directory.len);

		cherokee_buffer_drop_ending (&conn->local_directory,
		                             conn->pathinfo.len);
	}

	/* SCRIPT_FILENAME
	 * It depends on the type of CGI (CGI, SCGI o FastCGI):
	 *    http://php.net/reserved.variables
	 */
	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


static ret_t
mix_headers (cherokee_buffer_t *target,
             cherokee_buffer_t *source)
{
	char  tmp;
	char *begin;
	char *colon;
	char *end, *end1, *end2;

	begin = source->buf;
	while ((begin != NULL) && *begin)
	{
		end1 = strchr (begin, CHR_CR);
		end2 = strchr (begin, CHR_LF);

		end = cherokee_min_str (end1, end2);
		if (end == NULL) break;

		end2 = end;
		while ((*end2 == CHR_CR) || (*end2 == CHR_LF)) {
			end2++;
		}

		/* Begin points to the beginning of a new line
		 */
		tmp   = *end2;
		*end2 = '\0';
		colon = strstr (begin, ":");
		*end2 = tmp;

		if (colon != NULL) {
			tmp = colon[1];
			colon[1] = '\0';

			end1 = strcasestr (target->buf, begin);
			colon[1] = tmp;

			if (end1 == NULL) {
				cherokee_buffer_add     (target, begin, end-begin);
				cherokee_buffer_add_str (target, CRLF);
			}
		}

		begin = end2;
	}

	return ret_ok;
}


ret_t
cherokee_handler_cgi_base_extract_path (cherokee_handler_cgi_base_t *cgi,
                                        cherokee_boolean_t           check_filename)
{
	ret_t                              ret;
	cint_t                             req_len;
	cint_t                             local_len;
	struct stat                        nocache_info;
	struct stat                       *info;
	cherokee_iocache_entry_t          *io_entry     = NULL;
	cint_t                             pathinfo_len = 0;
	cherokee_connection_t             *conn         = HANDLER_CONN(cgi);
	cherokee_handler_cgi_base_props_t *props        = HANDLER_CGI_BASE_PROPS(cgi);
	cherokee_server_t                 *srv          = CONN_SRV(conn);

	/* ScriptAlias: If there is a ScriptAlias directive, it
	 * doesn't need to find the executable file..
	 */
	if (! cherokee_buffer_is_empty (&props->script_alias)) {
		TRACE (ENTRIES, "Script alias '%s'\n", props->script_alias.buf);

		ret = cherokee_io_stat (srv->iocache, &props->script_alias, props->use_cache, &nocache_info, &io_entry, &info);
		cherokee_iocache_entry_unref (&io_entry);
		if (ret != ret_ok) {
			conn->error_code = http_not_found;
			return ret_error;
		}

		cherokee_buffer_add_buffer (&cgi->executable, &props->script_alias);

		/* Check the path_info even if it uses a  scriptalias. The PATH_INFO
		 * is the rest of the substraction of request - configured directory.
		 */
		cherokee_connection_set_pathinfo (conn);
		return ret_ok;
	}

	/* No file checking: mainly for FastCGI and SCGI
	 *
	 * - If it's handling /, all the req. is path info
	 * - Otherwise, it is everything after conn->web_directory
	 */
	if (! props->check_file)
	{
		cherokee_connection_set_pathinfo (conn);
		return ret_ok;
	}

	req_len      = conn->request.len;
	local_len    = conn->local_directory.len;

	cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);

	/* Build the pathinfo string
	 */
	if (pathinfo_len <= 0) {
		cuint_t start = local_len - 1;

		if (! check_filename) {
			/* FastCGI and SCGI
			 */
			if (! cherokee_buffer_is_empty (&conn->web_directory)) {
				start += conn->web_directory.len;
			}

			ret = cherokee_handler_cgi_base_split_pathinfo (cgi, &conn->local_directory, start, true);
			if (ret != ret_ok) {
				/* It couldn't find a pathinfo, so let's check if there is something
				 * we can do here: skip the next file and the rest is pathinfo.
				 */
				char *end   = conn->local_directory.buf + conn->local_directory.len;
				char *begin = conn->local_directory.buf + start + 1;
				char *p     = begin;

				while ((p < end) && (*p != '/')) p++;

				if (p < end) {
					cherokee_buffer_add (&conn->pathinfo, p, end - p);

					pathinfo_len = end - p;
					cherokee_buffer_drop_ending (&conn->local_directory, pathinfo_len);
				}
			} else {
				pathinfo_len = conn->pathinfo.len;
			}
		}
		else {
			/* CGI and phpCGI
			 */
			ret = cherokee_handler_cgi_base_split_pathinfo (cgi, &conn->local_directory, start, false);
			if (unlikely(ret < ret_ok)) {
				conn->error_code = http_not_found;
				goto bye;
			}

			pathinfo_len = conn->pathinfo.len;
		}

		/* At this point:
		 *  - conn->pathinfo:        has been filled
		 *  - conn->local_directory: doesn't contain the pathinfo
		 */
	}

	TRACE (ENTRIES, "Pathinfo: '%s'\n", conn->pathinfo.buf);

	cherokee_buffer_add_buffer (&cgi->executable, &conn->local_directory);
	TRACE (ENTRIES, "Executable: '%s'\n", cgi->executable.buf);

	/* Check if the file exists
	 */
	ret = ret_ok;
	if (check_filename) {
		ret = cherokee_io_stat (srv->iocache, &conn->local_directory, props->use_cache, &nocache_info, &io_entry, &info);
		cherokee_iocache_entry_unref (&io_entry);
		if (ret != ret_ok) {
			conn->error_code = http_not_found;
			ret = ret_error;
			goto bye;
		}
	}

bye:
	/* Clean up the mess
	 */
	cherokee_buffer_drop_ending (&conn->local_directory, (req_len - pathinfo_len));
	return ret;
}


static ret_t
parse_header (cherokee_handler_cgi_base_t *cgi, cherokee_buffer_t *buffer)
{
	ret_t                  ret;
	char                  *end;
	char                  *end1;
	char                  *end2;
	char                  *begin;
	cherokee_connection_t *conn    = HANDLER_CONN(cgi);

	if (cherokee_buffer_is_empty (buffer) || buffer->len <= 5)
		return ret_ok;

	/* It is possible that the header ends with CRLF_CRLF
	 * In this case, we have to remove the last two characters
	 */
	if ((buffer->len > 4) &&
	    (strncmp (CRLF_CRLF, buffer->buf + buffer->len - 4, 4) == 0)) {
		cherokee_buffer_drop_ending (buffer, 2);
	}

	TRACE (ENTRIES, "CGI header: '%s'\n", buffer->buf);

	/* When an X-Sendfile header is send, we should not try to encode
	 * this file. In all likelihood the developer knows what happens,
	 * and might even have added Content-Length to the connection.
	 *
	 * This section should take preference over Content-Length parsing
	 * because it allows the Content-Length header to be added from CGI.
	 */

	if (HANDLER_CGI_BASE_PROPS(cgi)->allow_xsendfile) {
		begin = buffer->buf;
		while ((begin != NULL) && *begin)
		{
			end1 = strchr (begin, CHR_CR);
			end2 = strchr (begin, CHR_LF);

			end = cherokee_min_str (end1, end2);
			if (end == NULL) break;

			end2 = end;
			while ((*end2 == CHR_CR) || (*end2 == CHR_LF))
				end2++;

			if (begin[0] == 'X' || begin[0] == 'x') {
				if (strncasecmp ("X-Sendfile: ", begin, 12) == 0) {
					cherokee_buffer_add (&cgi->xsendfile, begin+12, end - (begin+12));
					cherokee_buffer_remove_chunk (buffer, begin - buffer->buf, end2 - begin);
					end2 = begin;

					TRACE (ENTRIES, "Found X-Sendfile header: '%s'\n", cgi->xsendfile.buf);

					BIT_SET (conn->options, conn_op_cant_encoder);
				}

				else if (strncasecmp ("X-Accel-Redirect: ", begin, 18) == 0)
				{
					cherokee_buffer_add (&cgi->xsendfile, begin+18, end - (begin+18));
					cherokee_buffer_remove_chunk (buffer, begin - buffer->buf, end2 - begin);
					end2 = begin;

					TRACE (ENTRIES, "Found X-Accel-Redirect header: '%s'\n", cgi->xsendfile.buf);

					BIT_SET (conn->options, conn_op_cant_encoder);
				}
			}

			begin = end2;
		}
	}

	/* Process the header line by line
	 */
	begin = buffer->buf;
	while ((begin != NULL) && *begin)
	{
		end1 = strchr (begin, CHR_CR);
		end2 = strchr (begin, CHR_LF);

		end = cherokee_min_str (end1, end2);
		if (end == NULL) break;

		end2 = end;
		while ((*end2 == CHR_CR) || (*end2 == CHR_LF))
			end2++;

		if (strncasecmp ("Status: ", begin, 8) == 0) {
			int  code;
			char status[4];

			memcpy (status, begin+8, 3);
			status[3] = '\0';

			ret = cherokee_atoi (status, &code);
			if ((ret != ret_ok) || (code < 100)) {
				conn->error_code = http_internal_error;
				return ret_error;
			}

			cherokee_buffer_remove_chunk (buffer, begin - buffer->buf, end2 - begin);
			end2 = begin;

			if (unlikely (conn->error_internal_code != http_unset)) {
				conn->error_internal_code = code;
			} else {
				conn->error_code = code;
			}

			continue;
		}

		else if (strncasecmp ("HTTP/", begin, 5) == 0) {
			int  code;
			char status[4];

			memcpy (status, begin+9, 3);
			status[3] = '\0';

			ret = cherokee_atoi (status, &code);
			if ((ret != ret_ok) || (code < 100)) {
				conn->error_code = http_internal_error;
				return ret_error;
			}

			cherokee_buffer_remove_chunk (buffer, begin - buffer->buf, end2 - begin);
			end2 = begin;

			if (unlikely (conn->error_internal_code != http_unset)) {
				conn->error_internal_code = code;
			} else {
				conn->error_code = code;
			}

			continue;
		}

		else if (strncasecmp ("Content-Length: ", begin, 16) == 0) {

			if (cherokee_connection_should_include_length(conn)) {
				char saved = *end;

				*end = '\0';
				cgi->content_length = strtoll (begin+16, (char **)NULL, 10);
				*end = saved;

				HANDLER(cgi)->support |= hsupport_length;
			}

			cherokee_buffer_remove_chunk (buffer, begin - buffer->buf, end2 - begin);
			end2 = begin;
		}

		else if (strncasecmp ("Location: ", begin, 10) == 0) {
			cherokee_buffer_add (&conn->redirect, begin+10, end - (begin+10));
			cherokee_buffer_remove_chunk (buffer, begin - buffer->buf, end2 - begin);
			end2 = begin;
		}

		else if (strncasecmp ("Content-Encoding: ", begin, 18) == 0) {
			BIT_SET (conn->options, conn_op_cant_encoder);
		}

		begin = end2;
	}

	return ret_ok;
}


ret_t
cherokee_handler_cgi_base_add_headers (cherokee_handler_cgi_base_t *cgi,
                                       cherokee_buffer_t           *outbuf)
{
	ret_t                  ret;
	int                    len;
	char                  *content;
	cuint_t                end_len;
	cherokee_buffer_t     *inbuf    = &cgi->data;
	cherokee_connection_t *conn     = HANDLER_CONN(cgi);

	/* Read some information
	 */
	ret = cgi->read_from_cgi (cgi, inbuf);
	switch (ret) {
	case ret_ok:
	case ret_eof_have_data:
		break;

	case ret_eof:
	case ret_error:
	case ret_eagain:
		return ret;

	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	/* Look the end of headers
	 */
	ret = cherokee_find_header_end (inbuf, &content, &end_len);
	switch (ret) {
	case ret_ok:
		break;
	case ret_error:
		return ret_error;
	default:
		return (cgi->got_eof) ? ret_eof : ret_eagain;
	}

	/* Copy the header
	 */
	len = content - inbuf->buf;

	cherokee_buffer_ensure_size (outbuf, len+6);
	cherokee_buffer_add (outbuf, inbuf->buf, len);
	cherokee_buffer_add_str (outbuf, CRLF_CRLF);

	/* Drop out the headers, we already have a copy
	 */
	cherokee_buffer_move_to_begin (inbuf, len + end_len);

	/* From this moment, it can handle errors
	 */
	if (HANDLER_CGI_BASE_PROPS(cgi)->is_error_handler) {
		HANDLER(cgi)->support |= hsupport_error;
	}

	/* Parse the header.. it is likely we will have something to do with it.
	 */
	ret = parse_header (cgi, outbuf);
	if (unlikely (ret != ret_ok)) {
		return ret;
	}

	/* Handle X-Sendfile
	 */
	if (! cherokee_buffer_is_empty (&cgi->xsendfile))
	{
		cherokee_buffer_t cgi_header = CHEROKEE_BUF_INIT;

		/* Instance the 'file' sub-handler
		 */
		handler_file_props.use_cache = true;
		ret = cherokee_handler_file_new ((cherokee_handler_t **) &cgi->file_handler,
		                                 conn, MODULE_PROPS(&handler_file_props));
		if (ret != ret_ok)
			return ret_error;

		ret = cherokee_handler_file_custom_init (cgi->file_handler, &cgi->xsendfile);
		if (ret != ret_ok)
			return ret_error;

		/* Work out the header
		 */
		cherokee_buffer_add_buffer (&cgi_header, outbuf);
		cherokee_buffer_clean (outbuf);

		ret = cherokee_handler_file_add_headers (cgi->file_handler, outbuf);
		if (ret != ret_ok) {
			cherokee_buffer_mrproper (&cgi_header);
			return ret_error;
		}

		/* Overwrite the handler properties
		 */
		HANDLER(cgi)->support  = HANDLER(cgi->file_handler)->support;
		conn->chunked_encoding = false;

		mix_headers (outbuf, &cgi_header);

		cherokee_buffer_mrproper (&cgi_header);
		return ret_ok;
	}

	/* Content-Length response header
	 */
	if (HANDLER_SUPPORTS (cgi, hsupport_length))
	{
		cherokee_buffer_add_str      (outbuf, "Content-Length: ");
		cherokee_buffer_add_ullong10 (outbuf, (cullong_t) cgi->content_length);
		cherokee_buffer_add_str      (outbuf, CRLF);
	}

	/* Redirection without custom status
	 */
	if ((conn->error_code == http_ok) &&
	    (! cherokee_buffer_is_empty (&conn->redirect)))
	{
		TRACE(ENTRIES, "Redirection without custom status. Setting %d\n", 302);
		conn->error_code = http_moved_temporarily;
	}

	return ret_ok;
}


ret_t
cherokee_handler_cgi_base_step (cherokee_handler_cgi_base_t *cgi,
                                cherokee_buffer_t           *outbuf)
{
	ret_t              ret;
	cherokee_buffer_t *inbuf = &cgi->data;

	/* Is it replying a "X-Sendfile" request?
	 */
	if (cgi->file_handler) {
		return cherokee_handler_file_step (cgi->file_handler, outbuf);
	}

	/* If there is some data waiting to be sent in the CGI buffer, move it
	 * to the connection buffer and continue..
	 */
	if (! cherokee_buffer_is_empty (&cgi->data)) {
		TRACE (ENTRIES, "sending stored data: %d bytes\n", cgi->data.len);

		cherokee_buffer_add_buffer (outbuf, &cgi->data);
		cherokee_buffer_clean (&cgi->data);

		if (cgi->got_eof) {
			return ret_eof_have_data;
		}

		return ret_ok;
	}

	/* Read some information from the CGI
	 */
	ret = cgi->read_from_cgi (cgi, inbuf);

	if (inbuf->len > 0) {
		cherokee_buffer_add_buffer (outbuf, inbuf);
		cherokee_buffer_clean (inbuf);
	}

	return ret;
}


ret_t
cherokee_handler_cgi_base_split_pathinfo (cherokee_handler_cgi_base_t *cgi,
                                          cherokee_buffer_t           *buf,
                                          int                          init_pos,
                                          int                          allow_dirs)
{
	ret_t                  ret;
	char                  *pathinfo;
	int                    pathinfo_len;
	cherokee_connection_t *conn = HANDLER_CONN(cgi);

	/* Look for the pathinfo
	 */
	ret = cherokee_split_pathinfo (buf, init_pos, allow_dirs, &pathinfo, &pathinfo_len);
	if (ret == ret_not_found)
		return ret;

	/* Split the string
	 */
	if (pathinfo_len > 0) {
		cherokee_buffer_add (&conn->pathinfo, pathinfo, pathinfo_len);
		cherokee_buffer_drop_ending (buf, pathinfo_len);
	}

	TRACE (ENTRIES, "Pathinfo '%s'\n", conn->pathinfo.buf);

	return ret_ok;
}
