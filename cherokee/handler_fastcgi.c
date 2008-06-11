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
#include "handler_fastcgi.h"
#include "fastcgi.h"
#include "util.h"
#include "thread.h"
#include "fcgi_dispatcher.h"

#include "connection-protected.h"

#define ENTRIES "handler,cgi"
#define POST_PACKAGE_LEN 32750
#define NSOCKS_DEFAULT   5

#define set_env(cgi,key,val,len) \
	set_env_pair (cgi, key, sizeof(key)-1, val, len)


/* Plug-in initialization
 */
CGI_LIB_INIT (fastcgi, http_get | http_post | http_head);


/* Methods implementation
 */
static ret_t 
props_free (cherokee_handler_fastcgi_props_t *props)
{
	if (props->balancer != NULL) 
		cherokee_balancer_free (props->balancer);

	/* TODO: Free the structure content
	 */
	return cherokee_handler_cgi_base_props_free (PROP_CGI_BASE(props));
}

ret_t 
cherokee_handler_fastcgi_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                             ret;
	cherokee_list_t                  *i, *j;
	cherokee_handler_fastcgi_props_t *props;

	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_fastcgi_props);
		
		cherokee_handler_cgi_base_props_init_base (PROP_CGI_BASE(n), 
							   MODULE_PROPS_FREE(props_free));		

		INIT_LIST_HEAD (&n->fastcgi_env_ref);

		n->balancer   = NULL;
		n->nsockets   = NSOCKS_DEFAULT;
		n->nkeepalive = 0;
		n->npipeline  = 0;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_FASTCGI(*_props);

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "balancer")) {
			ret = cherokee_balancer_instance (&subconf->val, subconf, srv, &props->balancer); 
			if (ret != ret_ok)
				return ret;

		} else if (equal_buf_str (&subconf->key, "fcgi_env")) {
			cherokee_config_node_foreach (j, conf) {
				cherokee_config_node_t *subconf2 = CONFIG_NODE(j);

				// TODO : implement this
				subconf2 = subconf2;
			}
			
		} else if (equal_buf_str (&subconf->key, "keepalive")) {
			props->nkeepalive = atoi(subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "pipeline")) {
			props->npipeline = atoi(subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "socket")) {
			props->nsockets = atoi(subconf->val.buf);
		}
	}
	return cherokee_handler_cgi_base_configure (conf, srv, _props);
}


static void
fcgi_build_header (FCGI_Header *hdr, cuchar_t type, cushort_t request_id, cuint_t content_length, cuchar_t padding)
{
	hdr->version         = FCGI_VERSION_1;
	hdr->type            = type;
	hdr->requestIdB0     = (cuchar_t) request_id;
	hdr->requestIdB1     = (cuchar_t) (request_id >> 8) & 0xff;
	hdr->contentLengthB0 = (cuchar_t) (content_length % 256);
	hdr->contentLengthB1 = (cuchar_t) (content_length / 256);
	hdr->paddingLength   = padding;
	hdr->reserved        = 0;
}

static void
fcgi_build_request_body (FCGI_BeginRequestRecord *request)
{
	request->body.roleB0      = FCGI_RESPONDER;
	request->body.roleB1      = 0;
	request->body.flags       = FCGI_KEEP_CONN;
	request->body.reserved[0] = 0;
	request->body.reserved[1] = 0;
	request->body.reserved[2] = 0;
	request->body.reserved[3] = 0;
	request->body.reserved[4] = 0;
}

static void 
set_env_pair (cherokee_handler_cgi_base_t *cgi_base, 
	      char *key, int key_len, 
	      char *val, int val_len)
{
	int                         len;
	FCGI_BeginRequestRecord     request;
	cherokee_handler_fastcgi_t *hdl = HDL_FASTCGI(cgi_base);	
	cherokee_buffer_t          *buf = &hdl->write_buffer;

	len  = key_len + val_len;
	len += key_len > 127 ? 4 : 1;
	len += val_len > 127 ? 4 : 1;

	fcgi_build_header (&request.header, FCGI_PARAMS, hdl->id, len, 0);

	cherokee_buffer_ensure_size (buf, buf->len + sizeof(FCGI_Header) + key_len + val_len);
	cherokee_buffer_add (buf, (void *)&request.header, sizeof(FCGI_Header));

	if (key_len <= 127) {
		buf->buf[buf->len++] = key_len;
	} else {
		buf->buf[buf->len++] = ((key_len >> 24) & 0xff) | 0x80;
		buf->buf[buf->len++] =  (key_len >> 16) & 0xff;
		buf->buf[buf->len++] =  (key_len >> 8)  & 0xff;
		buf->buf[buf->len++] =  (key_len >> 0)  & 0xff;
	}

	if (val_len <= 127) {
		buf->buf[buf->len++] = val_len;
	} else {
		buf->buf[buf->len++] = ((val_len >> 24) & 0xff) | 0x80;
		buf->buf[buf->len++] =  (val_len >> 16) & 0xff;
		buf->buf[buf->len++] =  (val_len >> 8)  & 0xff;
		buf->buf[buf->len++] =  (val_len >> 0)  & 0xff;
	}

	cherokee_buffer_add (buf, key, key_len);
	cherokee_buffer_add (buf, val, val_len);
}


static ret_t
read_from_fastcgi (cherokee_handler_cgi_base_t *cgi, cherokee_buffer_t *buffer)
{
	ret_t                       ret;
	cherokee_handler_fastcgi_t *hdl     = HDL_FASTCGI(cgi);
	cherokee_connection_t      *conn    = HANDLER_CONN(cgi);
	cherokee_fcgi_manager_t    *manager = hdl->manager;

	/* Maybe it received a ret_eof_have_data in _init
	 */
	if (cgi->got_eof)  {
		return (buffer->len > 0) ? ret_eof_have_data : ret_eof;
	}

	/* Does the connection belong to an old generation?
	 */
	if (hdl->generation != manager->generation) {
		TRACE (ENTRIES, "id=%d, different generation: conn=%d, manager=%d.. EOF\n", 
		       hdl->id, hdl->generation, manager->generation);

		return ret_eof;
	}
		
	/* Let the manager to some job
	 */
	ret = cherokee_fcgi_manager_step (manager);
	switch (ret) {
	case ret_ok:
		if (cgi->got_eof)  {
			return (buffer->len > 0) ? ret_eof_have_data : ret_eof;
		}

		if (cherokee_buffer_is_empty (buffer))
			return ret_eagain;

		TRACE (ENTRIES, "id=%d gen=%d, manager_step: OK\n", hdl->id, hdl->generation);
		return ret_ok;

	case ret_eagain:
		if (hdl->manager->pipeline > 0)
			cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl), conn, hdl->manager->socket.socket, 0, true);
		else
			cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl), conn, hdl->manager->socket.socket, 0, false);
			
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


ret_t
cherokee_handler_fastcgi_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_fastcgi);
	
	/* Init the base class
	 */
	cherokee_handler_cgi_base_init (HDL_CGI_BASE(n), cnt, PLUGIN_INFO_HANDLER_PTR(fastcgi), 
					HANDLER_PROPS(props), set_env_pair, read_from_fastcgi);

	/* Virtual methods
	 */
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_fastcgi_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_fastcgi_free;

	/* Virtual methods: implemented by handler_cgi_base
	 */
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_cgi_base_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_cgi_base_add_headers;

	/* Properties
	 */
	n->id         = 0xDEADBEEF;
	n->init_phase = fcgi_init_get_manager;
	n->post_phase = fcgi_post_init;
	n->post_len   = 0;
	n->manager    = NULL;
	n->dispatcher = NULL;

	cherokee_buffer_init (&n->write_buffer);

	/* The first FastCGI handler of each thread must create the
	 * FastCGI manager container table, and set the freeing func.
	 */
	if (CONN_THREAD(cnt)->fastcgi_servers == NULL) {
		CONN_THREAD(cnt)->fastcgi_free_func = (cherokee_func_free_t) cherokee_fcgi_dispatcher_free;
		cherokee_avl_new (&CONN_THREAD(cnt)->fastcgi_servers);
	}

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;	   
}


ret_t 
cherokee_handler_fastcgi_free (cherokee_handler_fastcgi_t *hdl)
{
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	if (hdl->manager != NULL) {
		cherokee_fcgi_manager_unregister (hdl->manager, conn);
	}

	cherokee_buffer_mrproper (&hdl->write_buffer);
	return ret_ok;
}


static ret_t
get_dispatcher (cherokee_handler_fastcgi_t *hdl, cherokee_fcgi_dispatcher_t **dispatcher)
{
	ret_t                             ret;
	cherokee_source_t                *src         = NULL;
	cherokee_thread_t                *thread      = HANDLER_THREAD(hdl);
	cherokee_avl_t                   *dispatchers = thread->fastcgi_servers;
	cherokee_handler_fastcgi_props_t *props       = HANDLER_FASTCGI_PROPS(hdl);

	/* Choose the server
	 */
	ret = cherokee_balancer_dispatch (props->balancer, HANDLER_CONN(hdl), &src);
	if (ret != ret_ok)
		return ret;

	/* Get the manager
	 */
	ret = cherokee_avl_get (dispatchers, &src->original, (void **)dispatcher);
	if (ret == ret_not_found) {
		ret = cherokee_fcgi_dispatcher_new (dispatcher, thread, src, props->nsockets, props->nkeepalive, props->npipeline);
		if (unlikely (ret != ret_ok))
			return ret;

		ret = cherokee_avl_add (dispatchers, &src->original, *dispatcher);
		if (unlikely (ret != ret_ok))
			return ret;
	}

	return ret_ok;
}


static ret_t
register_connection (cherokee_handler_fastcgi_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);
	cherokee_thread_t     *thd  = CONN_THREAD(conn);

	hdl->id         = 0;
	hdl->generation = 0;
	hdl->manager    = NULL;

	ret = cherokee_fcgi_dispatcher_dispatch (hdl->dispatcher, &hdl->manager);
	switch (ret) {
	case ret_ok:
		break;
	case ret_not_found:
		ret = cherokee_thread_retire_active_connection (thd, conn);
		if (unlikely (ret != ret_ok))
			return ret;

		ret = cherokee_fcgi_dispatcher_queue_conn (hdl->dispatcher, conn);
		if (unlikely (ret != ret_ok))
			return ret;

		return ret_eagain;
	default:
		return ret;
	}

	ret = cherokee_fcgi_manager_ensure_is_connected (hdl->manager, thd);
	if (unlikely (ret != ret_ok))
		return ret;

	ret = cherokee_fcgi_manager_register (hdl->manager, conn, &hdl->id, &hdl->generation);
	if (unlikely (ret != ret_ok))
		return ret;

	TRACE (ENTRIES, "disp=%p mgr=%p, ok\n", hdl->dispatcher, hdl->manager);

	return ret_ok;
}


static ret_t
add_extra_fastcgi_env (cherokee_handler_fastcgi_t *hdl, cuint_t *last_header_offset)
{
	ret_t                        ret;
	cherokee_handler_cgi_base_t *cgi_base = HDL_CGI_BASE(hdl);
	cherokee_buffer_t            buffer   = CHEROKEE_BUF_INIT;
	cherokee_connection_t       *conn     = HANDLER_CONN(hdl);

	/* CONTENT_LENGTH
	 */
	ret = cherokee_header_copy_known (&conn->header, header_content_length, &buffer);
	if (ret == ret_ok)
		set_env (cgi_base, "CONTENT_LENGTH", buffer.buf, buffer.len);

	/* PATH_INFO
	 */
	cherokee_buffer_clean (&buffer); 
	cherokee_buffer_add_buffer (&buffer, &conn->local_directory);
	if (conn->request.len > 0) {
		cherokee_buffer_add (&buffer, conn->request.buf + 1, conn->request.len - 1);
	}

/* 	if (! cherokee_buffer_is_empty (&conn->pathinfo)) {
 * 		set_env (cgi_base, "PATH_INFO", conn->pathinfo.buf, conn->pathinfo.len);
 * 	}
 */
	/* A few more..
	 */
	set_env (cgi_base, "PATH_TRANSLATED", buffer.buf, buffer.len);

	/* The last one
	 */
	*last_header_offset = hdl->write_buffer.len;
	set_env (cgi_base, "SCRIPT_FILENAME", buffer.buf, buffer.len);

	cherokee_buffer_mrproper (&buffer);
	return ret_ok;
}


static void
fixup_padding (cherokee_buffer_t *buf, cuint_t last_header_offset)
{
	cuint_t      rest;
	cuint_t      pad;
	static char  padding[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	FCGI_Header *last_header;

	if (buf->len <= 0)
		return;
	last_header = (FCGI_Header *) (buf->buf + last_header_offset);
	rest        = buf->len % 8;
	pad         = 8 - rest;

	if (rest == 0) 
		return;

	last_header->paddingLength = pad;

	cherokee_buffer_ensure_size (buf, buf->len + pad);
	cherokee_buffer_add (buf, padding, pad);
}


static void
add_empty_packet (cherokee_handler_fastcgi_t *hdl, cuint_t type)
{
	FCGI_BeginRequestRecord  request;
  
	fcgi_build_header (&request.header, type, hdl->id, 0, 0);
	cherokee_buffer_add (&hdl->write_buffer, (void *)&request.header, sizeof(FCGI_Header));

	TRACE (ENTRIES, "id=%d gen=%d, empty packet type=%d, len=%d\n", hdl->id, hdl->generation, type, hdl->write_buffer.len);
}


static ret_t
build_header (cherokee_handler_fastcgi_t *hdl)
{
	FCGI_BeginRequestRecord  request;
	cherokee_connection_t   *conn = HANDLER_CONN(hdl);
	cuint_t                  last_header_offset;

	/* Take care here, if the connection is reinjected, it
	 * shouldn't parse the arguments again.
	 */
	if (conn->arguments == NULL)
		cherokee_connection_parse_args (conn);

	cherokee_buffer_clean (&hdl->write_buffer);
	cherokee_buffer_ensure_size (&hdl->write_buffer, 512);

	/* FCGI_BEGIN_REQUEST
	 */	
	fcgi_build_header (&request.header, FCGI_BEGIN_REQUEST, hdl->id, sizeof(request.body), 0);
	fcgi_build_request_body (&request);

	cherokee_buffer_add (&hdl->write_buffer, (void *)&request, sizeof(FCGI_BeginRequestRecord));
	TRACE (ENTRIES, "id=%d gen=%d, Added FCGI_BEGIN_REQUEST, len=%d\n", hdl->id, hdl->generation, hdl->write_buffer.len);

	/* Add enviroment variables
	 */
	cherokee_handler_cgi_base_build_envp (HDL_CGI_BASE(hdl), conn);

	add_extra_fastcgi_env (hdl, &last_header_offset);
	fixup_padding (&hdl->write_buffer, last_header_offset);

	/* There are no more parameters
	*/
	add_empty_packet (hdl, FCGI_PARAMS);

	TRACE (ENTRIES, "id=%d gen=%d, Added FCGI_PARAMS, len=%d\n", hdl->id, hdl->generation, hdl->write_buffer.len);
	return ret_ok;
}


static ret_t
send_post (cherokee_handler_fastcgi_t *hdl, cherokee_buffer_t *buf)
{
	ret_t                    ret;
	cherokee_connection_t   *conn         = HANDLER_CONN(hdl);
	static FCGI_Header       empty_header = {0,0,0,0,0,0,0,0};

	switch (hdl->post_phase) {
	case fcgi_post_init:
		TRACE (ENTRIES, "id=%d gen=%d, Post phase = init\n", hdl->id, hdl->generation);

		/* Init the POST storing object
		 */
		cherokee_post_walk_reset (&conn->post);
		cherokee_post_get_len (&conn->post, &hdl->post_len);

		if (hdl->post_len <= 0)
			return ret_ok;
		
		hdl->post_phase = fcgi_post_read;

	case fcgi_post_read:
		TRACE (ENTRIES, "id=%d gen=%d, Post phase = read\n", hdl->id, hdl->generation);

		if (cherokee_buffer_is_empty (buf)) {
			cherokee_buffer_add (buf, (const char *)&empty_header, sizeof (FCGI_Header));
		}

		ret = cherokee_post_walk_read (&conn->post, buf, POST_PACKAGE_LEN);
		switch (ret) {
		case ret_ok:
		case ret_eagain:
			break;
		case ret_error:
			return ret;
		default:
			RET_UNKNOWN(ret);
			return ret_error;
		}

		if (buf->len > sizeof(FCGI_Header)) {
			fcgi_build_header ((FCGI_Header *)buf->buf, FCGI_STDIN, 
				hdl->id, buf->len - sizeof(FCGI_Header), 0);
		}

		ret = cherokee_post_walk_finished (&conn->post);
		if (ret == ret_ok) {
			add_empty_packet (hdl, FCGI_STDIN);
		}

		hdl->post_phase = fcgi_post_write;

	case fcgi_post_write:
		TRACE (ENTRIES, "id=%d gen=%d, Post phase = write, buf.len=%d\n", hdl->id, hdl->generation, buf->len);

		if (! cherokee_buffer_is_empty (buf)) {
			ret = cherokee_fcgi_manager_send_remove (hdl->manager, buf);
			switch (ret) {
				case ret_ok:
					break;
				case ret_eagain:
					return ret_eagain;
				case ret_eof:
				case ret_error:
					conn->error_code = http_bad_gateway;
					return ret_error;
				default:
					RET_UNKNOWN(ret);
					return ret_error;
			}
		}

		if (! cherokee_buffer_is_empty (buf))
			return ret_eagain;

		ret = cherokee_post_walk_finished (&conn->post);
		switch (ret) {
		case ret_ok:
			break;
		case ret_error:
			return ret_error;
		case ret_eagain:
			hdl->post_phase = fcgi_post_read;
			return ret_eagain;
		default:
			RET_UNKNOWN(ret);
			return ret_error;
		}

		TRACE (ENTRIES, "id=%d gen=%d, Post phase = finished\n", hdl->id, hdl->generation);
		return ret_ok;

	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}


ret_t
init_respin (cherokee_handler_fastcgi_t *hdl)
{
	ret_t ret;

	ret = cherokee_fcgi_manager_ensure_is_connected (hdl->manager, HANDLER_THREAD(hdl));
	if (unlikely (ret != ret_ok))
		return ret;
	
	ret = register_connection (hdl);
	if (unlikely (ret != ret_ok))
		return ret;
	
	HDL_CGI_BASE(hdl)->got_eof = false;
	hdl->init_phase = fcgi_init_build_header;
	return ret_eagain;			
}


ret_t 
cherokee_handler_fastcgi_init (cherokee_handler_fastcgi_t *hdl)
{
	ret_t                        ret;
	cherokee_connection_t       *conn = HANDLER_CONN(hdl);
	cherokee_handler_cgi_base_t *cgi  = HDL_CGI_BASE(hdl);

	switch (hdl->init_phase) {
	case fcgi_init_get_manager:
		TRACE (ENTRIES, "id=? gen=?, Init phase = %s\n", "get_manager");
		
		/* Choose a server to connect to
		 */
		if (hdl->dispatcher == NULL) {
			ret = get_dispatcher (hdl, &hdl->dispatcher);
			if (unlikely (ret != ret_ok)) return ret;
		}
		
		/* Register this connection with the manager
		 */
		ret = register_connection (hdl);
		switch (ret) {
		case ret_ok:
			break;
		case ret_not_found:
			conn->error_code = http_service_unavailable;
			return ret_error;
		default:
			return ret;
		}

		/* Set the executable filename
		 */
		ret = cherokee_handler_cgi_base_extract_path (HDL_CGI_BASE(cgi), true);
		if (unlikely (ret < ret_ok))
			return ret;

		hdl->init_phase = fcgi_init_build_header;

	case fcgi_init_build_header:
		TRACE (ENTRIES, "id=%d gen=%d, Init phase = build_header\n", hdl->id, hdl->generation);

		/* Build the initial header
		 */
		ret = build_header (hdl);
		if (unlikely (ret != ret_ok))
			return ret;

		hdl->init_phase = fcgi_init_send_header;

	case fcgi_init_send_header:
		TRACE (ENTRIES, "id=%d gen=%d, Init phase = send_header\n", hdl->id, hdl->generation);

		ret = cherokee_fcgi_manager_send_remove (hdl->manager, &hdl->write_buffer);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			return ret;
		case ret_eof:
			return init_respin (hdl);
		default:
			conn->error_code = http_bad_gateway;
			return ret;
		}
		
		hdl->init_phase = fcgi_init_send_post;

	case fcgi_init_send_post:
 		TRACE (ENTRIES, "id=%d gen=%d, Init phase = send_post\n", hdl->id, hdl->generation);

		ret = send_post (hdl, &hdl->write_buffer);
		if (ret != ret_ok)
			return ret;

		hdl->init_phase = fcgi_init_read;
		
	case fcgi_init_read:
 		TRACE (ENTRIES, "id=%d gen=%d, Init phase = read\n", hdl->id, hdl->generation);

		ret = read_from_fastcgi (HDL_CGI_BASE(hdl), &HANDLER_CONN(hdl)->buffer);
		switch (ret) {
		case ret_eof:
			if (cgi->got_eof) 
				break;

			return init_respin (hdl);

		case ret_error:
			return ret;

		case ret_ok:
		case ret_eagain:
			if (cgi->data.len > 0)
				break;
			
			return ret_eagain;
			
		default:
			break;
		}

		hdl->init_phase = fcgi_init_end;

	case fcgi_init_end:
		TRACE (ENTRIES, "id=%d gen=%d, Init finished\n", hdl->id, hdl->generation);
		return ret_ok;
		
	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}

