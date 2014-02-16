/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *	  Alvaro Lopez Ortega <alvaro@alobbs.com>
 *	  Stefan de Konink <stefan@konink.de>
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
#include "handler_zeromq.h"
#include "connection-protected.h"
#include "thread.h"
#include "util.h"

#define ENTRIES "zeromq"

PLUGIN_INFO_HANDLER_EASIEST_INIT (zeromq, http_post);

ret_t
cherokee_handler_zeromq_init (cherokee_handler_zeromq_t *hdl)
{
	ret_t							 ret;
	cherokee_buffer_t				*tmp   = &HANDLER_THREAD(hdl)->tmp_buf1;
	cherokee_connection_t			*conn  = HANDLER_CONN(hdl);
	cherokee_handler_zeromq_props_t *props = HANDLER_ZEROMQ_PROPS(hdl);

	/* We are going to look for gzipped encoding */
	cherokee_buffer_clean (tmp);
	ret = cherokee_header_copy_known (&conn->header, header_content_encoding, tmp);
	if (ret == ret_ok && cherokee_buffer_cmp_str(tmp, "gzip") == 0) {
		TRACE(ENTRIES, "ZeroMQ: incomming header '%s'\n", tmp->buf);
		return ret_ok;
	} else {
		cherokee_buffer_clean (tmp);
		ret = cherokee_header_copy_known (&conn->header, header_content_type, tmp);
		if (ret == ret_ok && cherokee_buffer_cmp_str(tmp, "application/gzip") == 0) {
			TRACE(ENTRIES, "ZeroMQ: incomming header '%s'\n", tmp->buf);
			return ret_ok;
		}
	}

	/* If we end up here that means content is plain, lets set up an encoder */
	ret = props->encoder_props->instance_func((void **)&hdl->encoder, props->encoder_props);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	ret = cherokee_encoder_init (hdl->encoder, conn);
	/* TODO: this is a great idea for KV78turbo, but being able to configure
	 * the reply (KV6, 15, 17) sounds like a good idea too.
	 */
	conn->error_code = http_no_content;
	return ret_error;
}

ret_t
cherokee_handler_zeromq_read_post (cherokee_handler_zeromq_t *hdl)
{
	zmq_msg_t				message;
	int						re;
	ret_t					ret;
	cherokee_buffer_t	   *post = &HANDLER_THREAD(hdl)->tmp_buf1;
	cherokee_connection_t  *conn = HANDLER_CONN(hdl);

	/* Check for the post info
	 */
	if (! conn->post.has_info) {
		conn->error_code = http_bad_request;
		return ret_error;
	}

	cherokee_buffer_clean (post);
	ret = cherokee_post_read (&conn->post, &conn->socket, post);
	switch (ret) {
	case ret_ok:
		cherokee_connection_update_timeout (conn);
		break;
	case ret_eagain:
		ret = cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl),
												   HANDLER_CONN(hdl),
												   conn->socket.socket,
												   FDPOLL_MODE_READ, false);
		if (ret != ret_ok) {
			return ret_error;
		} else {
			return ret_eagain;
		}
	default:
		conn->error_code = http_bad_request;
		return ret_error;
	}

	TRACE (ENTRIES, "Post contains: '%s'\n", post->buf);

	cherokee_buffer_add_buffer(&hdl->output, post);

	if (! cherokee_post_read_finished (&conn->post)) {
		return ret_eagain;
	} else {
		cherokee_buffer_t	 			*tmp   = &HANDLER_THREAD(hdl)->tmp_buf1;
		cherokee_handler_zeromq_props_t *props = HANDLER_ZEROMQ_PROPS(hdl);
		zmq_msg_t envelope;
		zmq_msg_t message;
		cuint_t len;

		if ((cherokee_buffer_is_empty (&conn->web_directory)) ||
			(cherokee_buffer_is_ending (&conn->web_directory, '/'))) {
			len = conn->web_directory.len;
		} else {
			len = conn->web_directory.len + 1;
		}

		cherokee_buffer_clean (tmp);
		cherokee_buffer_add   (tmp, conn->request.buf + len,
									conn->request.len - len);

		TRACE(ENTRIES, "ZeroMQ: incomming path '%s'\n", tmp->buf);

		zmq_msg_init_size (&envelope, tmp->len);
		memcpy (zmq_msg_data (&envelope), tmp->buf, tmp->len);

		if (hdl->encoder != NULL) {
			cherokee_encoder_flush(hdl->encoder, &hdl->output, &hdl->encoded);
			zmq_msg_init_size (&message, hdl->encoded.len);
			memcpy (zmq_msg_data (&message), hdl->encoded.buf, hdl->encoded.len);
		} else {
			zmq_msg_init_size (&message, hdl->output.len);
			memcpy (zmq_msg_data (&message), hdl->output.buf, hdl->output.len);
		}

		/* Atomic Section */
		CHEROKEE_MUTEX_LOCK (&props->mutex);
		zmq_msg_send (&envelope, props->socket, ZMQ_DONTWAIT | ZMQ_SNDMORE);
		zmq_msg_send (&message, props->socket, ZMQ_DONTWAIT);
		CHEROKEE_MUTEX_UNLOCK (&props->mutex);

		zmq_msg_close (&envelope);
		zmq_msg_close (&message);
	}

	return ret_ok;
}

ret_t
cherokee_handler_zeromq_add_headers (cherokee_handler_zeromq_t *hdl, cherokee_buffer_t *buffer)
{
	return ret_ok;
}

ret_t
cherokee_handler_zeromq_step (cherokee_handler_zeromq_t *hdl, cherokee_buffer_t *buffer)
{
	return ret_eof_have_data;
}

static ret_t
zeromq_free (cherokee_handler_zeromq_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->output);
	cherokee_buffer_mrproper (&hdl->encoded);
	
	if (hdl->encoder)
		cherokee_encoder_free (hdl->encoder);

	return ret_ok;
}

ret_t
cherokee_handler_zeromq_new (cherokee_handler_t	 **hdl, void *cnt, cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_zeromq);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(zeromq));

	MODULE(n)->init			= (handler_func_init_t) cherokee_handler_zeromq_init;
	MODULE(n)->free			= (module_func_free_t) zeromq_free;
	HANDLER(n)->read_post   = (handler_func_read_post_t) cherokee_handler_zeromq_read_post;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_zeromq_add_headers;
	HANDLER(n)->step		= (handler_func_step_t) cherokee_handler_zeromq_step;

	/* Supported features
	 */
	HANDLER(n)->support	 = hsupport_nothing;
	 
	cherokee_buffer_init (&n->output);
	cherokee_buffer_ensure_size (&n->output, 2097152);
	cherokee_buffer_init (&n->encoded);
	cherokee_buffer_ensure_size (&n->encoded, 2097152);
	n->encoder = NULL;

	*hdl = HANDLER(n);
	return ret_ok;
}

static ret_t 
props_free  (cherokee_handler_zeromq_props_t *props)
{
	zmq_close (props->socket);
	zmq_term (props->context);
	cherokee_buffer_mrproper (&props->endpoint);
	return ret_ok;
}

ret_t 
cherokee_handler_zeromq_configure (cherokee_config_node_t *conf,
								   cherokee_server_t *srv,
								   cherokee_module_props_t **_props)
{
	ret_t							ret;
	cherokee_list_t				 *i;
	cherokee_handler_zeromq_props_t *props;
	cherokee_plugin_info_t		  *info = NULL;
	
	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_zeromq_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n), 
						  MODULE_PROPS_FREE(props_free));
		cherokee_buffer_init (&n->endpoint);
		n->io_threads = 1;
		CHEROKEE_MUTEX_INIT (&n->mutex, CHEROKEE_MUTEX_FAST);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_ZEROMQ(*_props);
	
	/* Voodoo to get our own backend gzipper
	 */
	ret = cherokee_plugin_loader_get (&srv->loader, "gzip", &info);
	if (ret != ret_ok) {
		return ret;
	}

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "endpoint")) {
			cherokee_buffer_clean (&props->endpoint);
			cherokee_buffer_add_buffer (&props->endpoint, &subconf->val);
		} else if (equal_buf_str (&subconf->key, "io_threads")) {
			props->io_threads = atoi(subconf->val.buf);
		} else if (equal_buf_str (&subconf->key, "encoder") && info->configure) {
			encoder_func_configure_t configure = info->configure;
			props->encoder_props = NULL;

			ret = configure (subconf, srv, (cherokee_module_props_t **)&props->encoder_props);
			if (ret != ret_ok) {
				return ret;
			}

			props->encoder_props->instance_func = PLUGIN_INFO(info)->instance;
		}

	}
	
	props->context = zmq_init (props->io_threads);
	
	props->socket = zmq_socket(props->context, ZMQ_PUSH);
	if (zmq_connect (props->socket, props->endpoint.buf) != 0) {
		TRACE(ENTRIES, "ZeroMQ Connect: can't connect to '%s'\n", props->endpoint.buf);
		return ret_error;
	}
	
	return ret_ok;
}
