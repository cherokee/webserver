/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *	  Alvaro Lopez Ortega <alvaro@alobbs.com>
 *	  Stefan de Konink <stefan@konink.de>
 *
 * Copyright (C) 2001-2013 Alvaro Lopez Ortega
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
#include "handler_tmi.h"
#include "connection-protected.h"
#include "thread.h"
#include "util.h"
#include "bogotime.h"
#include "zlib.h"

#define ENTRIES "tmi"

PLUGIN_INFO_HANDLER_EASIEST_INIT (tmi, http_post);

ret_t
cherokee_handler_tmi_init (cherokee_handler_tmi_t *hdl)
{
	ret_t ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);
	cherokee_buffer_t	 *tmp  = &HANDLER_THREAD(hdl)->tmp_buf1;
	cherokee_handler_tmi_props_t *props = HANDLER_TMI_PROPS(hdl);
		
	/* We are going to look for gzipped encoding */
	cherokee_buffer_clean (tmp);
	ret = cherokee_header_copy_known (&conn->header, header_content_encoding, tmp);
	if (ret == ret_ok && cherokee_buffer_cmp_str(tmp, "gzip") == 0) {
		TRACE(ENTRIES, "ZeroMQ: incomming header '%s'\n", tmp->buf);
		hdl->inflated = true;
	} else {
		cherokee_buffer_clean (tmp);
		ret = cherokee_header_copy_known (&conn->header, header_content_type, tmp);
	   	if (ret == ret_ok && (cherokee_buffer_cmp_str(tmp, "application/gzip") == 0 || cherokee_buffer_cmp_str(tmp, "application/zip") == 0)) {
			TRACE(ENTRIES, "ZeroMQ: incomming header '%s'\n", tmp->buf);
			hdl->inflated = true;
		} else {
			hdl->inflated = false;
		}
	}

#ifdef LIBXML_PUSH_ENABLED
	if (props->validate_xml) {
		hdl->validate_xml = true;
		hdl->ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL);
		xmlCtxtUseOptions(hdl->ctxt, XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET | XML_PARSE_COMPACT);

		if (hdl->inflated) {
			/* allocate inflate state */
			hdl->strm.zalloc = Z_NULL;
			hdl->strm.zfree = Z_NULL;
			hdl->strm.opaque = Z_NULL;
			hdl->strm.avail_in = 0;
			hdl->strm.next_in = Z_NULL;
			hdl->z_ret = inflateInit2(&(hdl->strm), 16+MAX_WBITS);
			if (hdl->z_ret != Z_OK)
				hdl->validate_xml = false;
		}
	}
#endif

	if (!hdl->inflated) {
		/* If we end up here that means content is plain, lets set up an encoder */
		ret = props->encoder_props->instance_func((void **)&hdl->encoder, props->encoder_props);
		if (unlikely (ret != ret_ok)) {
			return ret_error;
		}

		ret = cherokee_encoder_init (hdl->encoder, conn);
		if (unlikely (ret != ret_ok)) {
			return ret_error;
		}
	}

	return ret_ok;
}

ret_t
cherokee_handler_tmi_read_post (cherokee_handler_tmi_t *hdl)
{
	zmq_msg_t message;
	int					  re;
	ret_t					ret;
	ret_t					ret_final;
	cherokee_buffer_t	   *post = &HANDLER_THREAD(hdl)->tmp_buf1;
	cherokee_buffer_t	   *encoded = &HANDLER_THREAD(hdl)->tmp_buf2;
	cherokee_connection_t   *conn = HANDLER_CONN(hdl);

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

	re = cherokee_post_read_finished (&conn->post);
	ret_final = re ? ret_ok : ret_eagain;

	cherokee_buffer_clean(encoded);
	if (hdl->encoder != NULL) {
	   	if (ret == ret_ok) {
			cherokee_encoder_flush(hdl->encoder, post, encoded);
		} else {
			cherokee_encoder_encode(hdl->encoder, post, encoded);
		}
	} else {
		encoded = post;
	}

	cherokee_buffer_add_buffer(&hdl->output, post);

	if (ret_final == ret_ok) {
		cherokee_buffer_t	 *tmp  = &HANDLER_THREAD(hdl)->tmp_buf1;
		cherokee_handler_tmi_props_t *props = HANDLER_TMI_PROPS(hdl);
		zmq_msg_t envelope;
		zmq_msg_t message;
		cuint_t len;

		if ((cherokee_buffer_is_empty (&conn->web_directory)) ||
			(cherokee_buffer_is_ending (&conn->web_directory, '/')))
		{
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
		zmq_msg_init_size (&message, hdl->output.len);
		memcpy (zmq_msg_data (&message), hdl->output.buf, hdl->output.len);

		/* Atomic Section */
		CHEROKEE_MUTEX_LOCK (&props->mutex);
		zmq_msg_send (&envelope, props->socket, ZMQ_DONTWAIT | ZMQ_SNDMORE);
		zmq_msg_send (&message, props->socket, ZMQ_DONTWAIT);
		CHEROKEE_MUTEX_UNLOCK (&props->mutex);

		zmq_msg_close (&envelope);
		zmq_msg_close (&message);
		
#ifdef LIBXML_PUSH_ENABLED
		if (hdl->validate_xml) {
			if (hdl->inflated) {
				hdl->strm.avail_in = hdl->output.len;
				hdl->strm.next_in = hdl->output.buf;
					
				/* run inflate() on input until output buffer not full */
				do  {
					#define CHUNK 131072
					int have;
					char out[CHUNK];
					hdl->strm.avail_out = CHUNK;
					hdl->strm.next_out = out;
					hdl->z_ret = inflate(&(hdl->strm), Z_NO_FLUSH);
					switch (hdl->z_ret) {
					case Z_NEED_DICT:
						hdl->z_ret = Z_DATA_ERROR;	 /* and fall through */
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
					case Z_STREAM_ERROR:
						hdl->z_ret = Z_STREAM_ERROR;
						return ret_ok;
					}
					have = CHUNK - hdl->strm.avail_out;
					xmlParseChunk(hdl->ctxt, out, have, 0);
				} while (hdl->strm.avail_out == 0);
			} else {
				xmlParseChunk(hdl->ctxt, hdl->output.buf, hdl->output.len, 0);
			}
		}
#endif
	}

	return ret_final;
}

ret_t
cherokee_handler_tmi_add_headers (cherokee_handler_tmi_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_str (buffer, "Content-Type: application/xml" CRLF);
	return ret_ok;
}

ret_t
cherokee_handler_tmi_step (cherokee_handler_tmi_t *hdl, cherokee_buffer_t *buffer)
{
	char time_buf[21];
	size_t len;
	
	len = strftime(time_buf, 21, "%Y-%m-%dT%H:%S:%MZ", &cherokee_bogonow_tmgmt);
	cherokee_buffer_add_buffer (buffer, &HANDLER_TMI_PROPS(hdl)->reply);
	cherokee_buffer_add (buffer, time_buf, len);
	cherokee_buffer_add_str (buffer, "</tmi8:Timestamp><tmi8:ResponseCode>");

#ifdef LIBXML_PUSH_ENABLED
	if (hdl->validate_xml) {
		if (hdl->inflated && hdl->z_ret != Z_OK) {
			cherokee_buffer_add_str(buffer, "PE");
		} else {
			xmlParseChunk(hdl->ctxt, NULL, 0, 1);
			if (hdl->ctxt->wellFormed) {
				cherokee_buffer_add_str(buffer, "OK");
			} else {
				cherokee_buffer_add_str(buffer, "SE");
			}
		}
	}
#endif

	cherokee_buffer_add_str (buffer, "</tmi8:ResponseCode></tmi8:VV_TM_RES>");

	return ret_eof_have_data;
}

static ret_t
tmi_free (cherokee_handler_tmi_t *hdl)
{
	cherokee_handler_tmi_props_t *props = HANDLER_TMI_PROPS(hdl);
	cherokee_buffer_mrproper (&hdl->output);
	
	if (hdl->encoder)
		cherokee_encoder_free (hdl->encoder);

#ifdef LIBXML_PUSH_ENABLED
	if (props->validate_xml) {
		if (hdl->inflated)
			(void)inflateEnd(&(hdl->strm));

		xmlFreeDoc(hdl->ctxt->myDoc);
		xmlFreeParserCtxt(hdl->ctxt);
	}
#endif

	return ret_ok;
}

ret_t
cherokee_handler_tmi_new (cherokee_handler_t **hdl, 
						  void *cnt,
						  cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_tmi);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(tmi));

	MODULE(n)->init		 = (handler_func_init_t) cherokee_handler_tmi_init;
	MODULE(n)->free		 = (module_func_free_t) tmi_free;
	HANDLER(n)->read_post   = (handler_func_read_post_t) cherokee_handler_tmi_read_post;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_tmi_add_headers;
	HANDLER(n)->step		= (handler_func_step_t) cherokee_handler_tmi_step;

	/* Supported features
	 */
	HANDLER(n)->support	 = hsupport_nothing;
	
	cherokee_buffer_init (&n->output);
	cherokee_buffer_ensure_size (&n->output, 2097152);
	n->encoder = NULL;
	n->validate_xml = false;

	*hdl = HANDLER(n);
	return ret_ok;
}

static ret_t 
props_free  (cherokee_handler_tmi_props_t *props)
{
	zmq_close (props->socket);
	zmq_term (props->context);
	cherokee_buffer_mrproper (&props->reply);
	cherokee_buffer_mrproper (&props->subscriberid);
	cherokee_buffer_mrproper (&props->version);
	cherokee_buffer_mrproper (&props->dossiername);
	cherokee_buffer_mrproper (&props->endpoint);
	return ret_ok;
}

ret_t 
cherokee_handler_tmi_configure (cherokee_config_node_t  *conf, 
					 cherokee_server_t	   *srv,
					 cherokee_module_props_t **_props)
{
	ret_t						  ret;
	cherokee_list_t				 *i;
	cherokee_handler_tmi_props_t *props;
	cherokee_plugin_info_t		 *info = NULL;
	
	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_tmi_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n), 
						  MODULE_PROPS_FREE(props_free));
		
		cherokee_buffer_init (&n->reply);
		cherokee_buffer_init (&n->subscriberid);
		cherokee_buffer_init (&n->version);
		cherokee_buffer_init (&n->dossiername);
		cherokee_buffer_init (&n->endpoint);
		n->io_threads = 1;
		n->validate_xml = false;
		CHEROKEE_MUTEX_INIT (&n->mutex, CHEROKEE_MUTEX_FAST);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_TMI(*_props);
	
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
		
		if (equal_buf_str (&subconf->key, "subscriberid")) {
			cherokee_buffer_clean (&props->subscriberid);
			cherokee_buffer_add_buffer (&props->subscriberid, &subconf->val);
		} else if (equal_buf_str (&subconf->key, "version")) {
			cherokee_buffer_clean (&props->version);
			cherokee_buffer_add_buffer (&props->version, &subconf->val);
		} else if (equal_buf_str (&subconf->key, "dossiername")) {
			cherokee_buffer_clean (&props->dossiername);
			cherokee_buffer_add_buffer (&props->dossiername, &subconf->val);
		} else if (equal_buf_str (&subconf->key, "endpoint")) {
			cherokee_buffer_clean (&props->endpoint);
			cherokee_buffer_add_buffer (&props->endpoint, &subconf->val);
		} else if (equal_buf_str (&subconf->key, "io_threads")) {
			props->io_threads = atoi(subconf->val.buf);
		} else if (equal_buf_str (&subconf->key, "validate_xml")) {
			cherokee_atob (subconf->val.buf, &props->validate_xml);
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

	cherokee_buffer_clean (&props->reply);
	cherokee_buffer_add_va(&props->reply, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" 
										  "<tmi8:VV_TM_RES xmlns:tmi8=\"http://bison.connekt.nl/tmi8/kv6/msg\">" 
										  "<tmi8:SubscriberID>%s</tmi8:SubscriberID>"
										  "<tmi8:Version>%s</tmi8:Version>"
										  "<tmi8:DossierName>%s</tmi8:DossierName>"
										  "<tmi8:Timestamp>", props->subscriberid.buf, props->version.buf, props->dossiername.buf);

	return ret_ok;
}
