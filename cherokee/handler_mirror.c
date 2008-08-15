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
#include "handler_mirror.h"
#include "header-protected.h"
#include "connection-protected.h"
#include "thread.h"
#include "source.h"

#define ENTRIES "handler,mirror"

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (mirror, http_all_methods);



ret_t 
cherokee_handler_mirror_props_free (cherokee_handler_mirror_props_t *props)
{
	if (props->balancer) {
		cherokee_balancer_free (props->balancer);
	}

	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}

ret_t 
cherokee_handler_mirror_configure (cherokee_config_node_t   *conf,
				   cherokee_server_t        *srv, 
				   cherokee_module_props_t **_props)
{
	ret_t                            ret;
	cherokee_list_t                 *i;
	cherokee_handler_mirror_props_t *props;
	
	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_mirror_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n), 
			MODULE_PROPS_FREE(cherokee_handler_mirror_props_free));

		n->balancer = NULL;
		*_props = MODULE_PROPS(n);
	}

	props = PROP_MIRROR(*_props);	

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "balancer")) {
			ret = cherokee_balancer_instance (&subconf->val, subconf, srv, &props->balancer); 
			if (ret != ret_ok) return ret;
		} else {
			PRINT_MSG ("ERROR: Handler mirror: Unknown key: '%s'\n", subconf->key.buf);
			return ret_deny;
		}
	}

	/* Final checks
	 */
	if (props->balancer == NULL) {
		PRINT_ERROR_S ("ERROR: Mirror handler needs a balancer\n");
		return ret_error;
	}

	return ret_ok;
}


ret_t 
cherokee_handler_mirror_new  (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_mirror);

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(mirror));

	MODULE(n)->free         = (module_func_free_t) cherokee_handler_mirror_free;
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_mirror_init;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_mirror_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_mirror_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_skip_headers;

	/* Init
	 */
	n->header_sent = 0;
	n->post_sent   = 0;
	n->post_len    = 0;
	n->src_ref     = NULL;
	n->phase       = hmirror_phase_connect;

	cherokee_socket_init (&n->socket);
	
	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t 
cherokee_handler_mirror_free (cherokee_handler_mirror_t *hdl)
{
	cherokee_socket_close (&hdl->socket);
	cherokee_socket_mrproper (&hdl->socket);

	return ret_ok;
}


static ret_t
connect_to_server (cherokee_handler_mirror_t *hdl)
{
	ret_t                            ret;
	cherokee_connection_t           *conn     = HANDLER_CONN(hdl);
	cherokee_handler_mirror_props_t *props    = HDL_MIRROR_PROPS(hdl);

	/* Pick a host
	 */
	if (hdl->src_ref == NULL) {
		ret = cherokee_balancer_dispatch (props->balancer, conn, &hdl->src_ref);
		if (ret != ret_ok)
			return ret;
	}

	/* Try to connect
	 */
	return cherokee_source_connect_polling (hdl->src_ref, &hdl->socket, conn);
}


static ret_t
send_header (cherokee_handler_mirror_t *hdl)
{
	ret_t                  ret;
	size_t                 written = 0;
	cherokee_connection_t *conn    = HANDLER_CONN(hdl);
	cherokee_buffer_t     *header  = conn->header.input_buffer;

	if (hdl->header_sent >= header->len)
		return ret_ok;

	ret = cherokee_socket_bufwrite (&hdl->socket, header, &written);
	if (ret != ret_ok) {
		conn->error_code = http_bad_gateway;
		return ret;
	}
	
	hdl->header_sent += written;
	TRACE (ENTRIES, "sent %d, remaining=%d\n", written, header->len - hdl->header_sent);

	if (hdl->header_sent < header->len)
		return ret_eagain;
	
	return ret_ok;
}


static ret_t
send_post (cherokee_handler_mirror_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	ret = cherokee_post_walk_to_fd (&conn->post, hdl->socket.socket, NULL, NULL);
	switch (ret) {
	case ret_ok:
		break;
	default:
		conn->error_code = http_bad_gateway;
		return ret_error;
	}

	return ret_ok;
}


ret_t 
cherokee_handler_mirror_init (cherokee_handler_mirror_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	switch (hdl->phase) {
	case hmirror_phase_connect:
		TRACE(ENTRIES, "Connect begins %s", "\n");

		/* Connect to the remote server
		 */
		ret = connect_to_server (hdl);
		if (unlikely(ret != ret_ok)) return ret;

		/* Prepare Post
		 */
		if (! cherokee_post_is_empty (&conn->post)) {
			cherokee_post_walk_reset (&conn->post);
			cherokee_post_get_len (&conn->post, &hdl->post_len);
		}

		hdl->phase = hmirror_phase_send_headers;

	case hmirror_phase_send_headers:
		TRACE(ENTRIES, "Send headers begins %s", "\n");

		/* Send the header
		 */
		ret = send_header (hdl);
		if (ret != ret_ok) return ret;

		hdl->phase = hmirror_phase_send_post;

	case hmirror_phase_send_post:
		TRACE(ENTRIES, "Send post len=%d\n", hdl->post_len);

		/* Send the post, if any
		 */
		if ((hdl->post_len > 0) && 
		    (hdl->post_len > hdl->post_sent)) 
		{
			ret = send_post (hdl);
			if (ret != ret_ok) return ret;
		}

		break;
	}

	TRACE(ENTRIES, "finished: %s\n", "ret_ok");
	return ret_ok;
}


ret_t 
cherokee_handler_mirror_add_headers (cherokee_handler_mirror_t *hdl, cherokee_buffer_t *buffer)
{
	UNUSED(hdl);
	UNUSED(buffer);

	TRACE(ENTRIES, "does nothing: %s\n", "ret_ok");

	/* do nothing */
	return ret_ok;
}


ret_t
cherokee_handler_mirror_step (cherokee_handler_mirror_t *hdl, cherokee_buffer_t *buffer)
{
	ret_t  ret;
	size_t got = 0;

	ret = cherokee_socket_bufread (&hdl->socket, buffer, DEFAULT_READ_SIZE, &got);
	switch (ret) {
	case ret_ok:
		TRACE (ENTRIES, "%d bytes read\n", got);
		return ret_ok;

	case ret_eagain:
		cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl), 
						     HANDLER_CONN(hdl), 
						     hdl->socket.socket, 
						     FDPOLL_MODE_READ,
						     false);
		return ret_eagain;

	case ret_eof:
	case ret_error:
		return ret;

	default:
		RET_UNKNOWN(ret);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}

