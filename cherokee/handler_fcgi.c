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

#include <fcntl.h>

#include "handler_fcgi.h"
#include "header.h"
#include "connection-protected.h"
#include "util.h"
#include "thread.h"
#include "source_interpreter.h"
#include "bogotime.h"

#include "fastcgi.h"

#define POST_PACKAGE_LEN 32600
#define ENTRIES "fcgi,handler"


#define set_env(cgi,key,val,len) \
	set_env_pair (cgi, key, sizeof(key)-1, val, len)


static void set_env_pair (cherokee_handler_cgi_base_t *cgi_base, 
			  char *key, int key_len, 
			  char *val, int val_len);

/* Plug-in initialization
 */
CGI_LIB_INIT (fcgi, http_get | http_post | http_head);


/* Methods implementation
 */
static ret_t
process_package (cherokee_handler_fcgi_t *hdl, cherokee_buffer_t *inbuf, cherokee_buffer_t *outbuf)
{
	FCGI_Header           *header;
	FCGI_EndRequestBody   *ending;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	cuint_t  len;
	char    *data;
	cint_t   return_val;
	cuint_t  type;
	cuint_t  id;
	cuint_t  padding;
		
	/* Is there enough information?
	 */
	if (inbuf->len < sizeof(FCGI_Header))
		return ret_ok;

	/* At least there is a header
	 */
	header = (FCGI_Header *)inbuf->buf;

	if (header->version != 1) {
		cherokee_buffer_print_debug (inbuf, -1);
		PRINT_ERROR_S ("Parsing error: unknown version\n");
		return ret_error;
	}
	
	if (header->type != FCGI_STDERR &&
	    header->type != FCGI_STDOUT && 
	    header->type != FCGI_END_REQUEST) {
		cherokee_buffer_print_debug (inbuf, -1);
		PRINT_ERROR_S ("Parsing error: unknown type\n");
		return ret_error;
	}
	
	/* Read the header
	 */
	type    =  header->type;
	padding =  header->paddingLength;
	id      = (header->requestIdB0     | (header->requestIdB1 << 8));
	len     = (header->contentLengthB0 | (header->contentLengthB1 << 8));
	data    = inbuf->buf +  FCGI_HEADER_LEN;

/*	printf ("have %d, hdr=%d exp_len=%d pad=%d\n", inbuf->len, FCGI_HEADER_LEN, len, padding); */

	/* Is the package complete?
	 */
	if (len + padding > inbuf->len - FCGI_HEADER_LEN) {
/*		printf ("Incomplete: %d < %d\n", len + padding, inbuf->len - FCGI_HEADER_LEN); */
		return ret_ok;
	}

	/* It has received the full package content
	 */
	switch (type) {
	case FCGI_STDERR: 
/*		printf ("READ:STDERR (%d): %s", len, data?data:""); */

		if (CONN_VSRV(conn)->logger != NULL) {
			cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;
			
			cherokee_buffer_add (&tmp, data, len);
			cherokee_logger_write_string (CONN_VSRV(conn)->logger, "%s", tmp.buf);
			PRINT_ERROR ("%s\n", tmp.buf);
			cherokee_buffer_mrproper (&tmp);
		}
		break;

	case FCGI_STDOUT:
/*		printf ("READ:STDOUT eof=%d: %d", CGI_BASE(hdl)->got_eof, len); */
		cherokee_buffer_add (outbuf, data, len);
		break;

	case FCGI_END_REQUEST: 
		ending = (FCGI_EndRequestBody *)data;

		return_val = ((ending->appStatusB0)       | 
			      (ending->appStatusB0 << 8)  | 
			      (ending->appStatusB0 << 16) | 
			      (ending->appStatusB0 << 24));

		HDL_CGI_BASE(hdl)->got_eof = true;
/*		printf ("READ:END"); */
		break;

	default:
		SHOULDNT_HAPPEN;
	}

	cherokee_buffer_move_to_begin (inbuf, len + FCGI_HEADER_LEN + padding);
/*	printf ("- FCGI quedan %d\n", inbuf->len); */
	return ret_eagain;
}


static ret_t
process_buffer (cherokee_handler_fcgi_t *hdl, cherokee_buffer_t *inbuf, cherokee_buffer_t *outbuf)
{
	ret_t ret;
	
	do {
		ret = process_package (hdl, inbuf, outbuf);
	} while (ret == ret_eagain);

	if (ret == ret_ok) {
		if (cherokee_buffer_is_empty (outbuf))
			return (HDL_CGI_BASE(hdl)->got_eof) ? ret_eof : ret_eagain;
	}

	return ret;
}

static ret_t 
read_from_fcgi (cherokee_handler_cgi_base_t *cgi, cherokee_buffer_t *buffer)
{
	ret_t                    ret;
	size_t                   read = 0;
	cherokee_handler_fcgi_t *fcgi = HDL_FCGI(cgi);
	
	ret = cherokee_socket_bufread (&fcgi->socket, &fcgi->write_buffer, DEFAULT_READ_SIZE, &read);

	switch (ret) {
	case ret_eagain:
		cherokee_thread_deactive_to_polling (HANDLER_THREAD(cgi), HANDLER_CONN(cgi), 
						     fcgi->socket.socket, 0, false);
		return ret_eagain;

	case ret_ok:
		ret = process_buffer (fcgi, &fcgi->write_buffer, buffer);
		TRACE (ENTRIES, "%d bytes read, buffer.len %d\n", read, buffer->len);
		if ((ret == ret_ok) && cgi->got_eof && (buffer->len > 0)) 
			return ret_eof_have_data;
		return ret;

	case ret_eof:
	case ret_error:
		cgi->got_eof = true;
		return ret;

	default:
		RET_UNKNOWN(ret);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


static ret_t 
props_free (cherokee_handler_fcgi_props_t *props)
{
	if (props->balancer != NULL) 
		cherokee_balancer_free (props->balancer);
	
	return cherokee_handler_cgi_base_props_free (PROP_CGI_BASE(props));
}


ret_t 
cherokee_handler_fcgi_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                          ret;
	cherokee_list_t               *i;
	cherokee_handler_fcgi_props_t *props;

	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_fcgi_props);

		cherokee_handler_cgi_base_props_init_base (PROP_CGI_BASE(n),
							   MODULE_PROPS_FREE(props_free));

		INIT_LIST_HEAD (&n->server_list);
		n->balancer = NULL;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_FCGI(*_props);	

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "balancer")) {
			ret = cherokee_balancer_instance (&subconf->val, subconf, srv, &props->balancer); 
			if (ret != ret_ok) return ret;
		}
	}
	
	/* Init base class
	 */
	ret = cherokee_handler_cgi_base_configure (conf, srv, _props);
	if (ret != ret_ok) return ret;

	/* Final checks
	 */
	if (props->balancer == NULL) {
		PRINT_ERROR_S ("ERROR: fcgi handler needs a balancer\n");
		return ret_error;
	}

	return ret_ok;
}


ret_t 
cherokee_handler_fcgi_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_fcgi);

	/* Init the base class
	 */
	cherokee_handler_cgi_base_init (HDL_CGI_BASE(n), cnt, PLUGIN_INFO_HANDLER_PTR(fcgi), 
					HANDLER_PROPS(props), set_env_pair, read_from_fcgi);
	
	/* Virtual methods
	 */
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_fcgi_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_fcgi_free;

	/* Virtual methods: implemented by handler_cgi_base
	 */
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_cgi_base_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_cgi_base_add_headers;

	/* Properties
	 */
	n->post_phase  = fcgi_post_init;
	n->post_len    = 0;
	n->src_ref     = NULL;
	n->spawned     = 0;

	cherokee_socket_init (&n->socket);
	cherokee_buffer_init (&n->write_buffer);
	cherokee_buffer_ensure_size (&n->write_buffer, 512);

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;	   
}


ret_t 
cherokee_handler_fcgi_free (cherokee_handler_fcgi_t *hdl)
{
	TRACE (ENTRIES, "fcgi handler free: %p\n", hdl);

	cherokee_socket_close (&hdl->socket);
	cherokee_socket_mrproper (&hdl->socket);

	cherokee_buffer_mrproper (&hdl->write_buffer);

	return cherokee_handler_cgi_base_free (HDL_CGI_BASE(hdl));
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
	request->body.flags       = 0;
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
	int                       len;
	FCGI_BeginRequestRecord   request;
	cherokee_handler_fcgi_t  *hdl = HDL_FCGI(cgi_base);	
	cherokee_buffer_t        *buf = &hdl->write_buffer;

	len  = key_len + val_len;
	len += key_len > 127 ? 4 : 1;
	len += val_len > 127 ? 4 : 1;

	fcgi_build_header (&request.header, FCGI_PARAMS, 1, len, 0);

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
add_extra_fcgi_env (cherokee_handler_fcgi_t *hdl, cuint_t *last_header_offset)
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

	/* Add PATH_TRANSLATED only it there is pathinfo
	 */
#if 0
	if (! cherokee_buffer_is_empty (&conn->pathinfo)) {
		cherokee_buffer_add_buffer (&buffer, &conn->local_directory); 
		cherokee_buffer_add_buffer (&buffer, &conn->pathinfo);

		set_env (cgi_base, "PATH_TRANSLATED", buffer.buf, buffer.len);
		TRACE (ENTRIES, "PATH_TRANSLATED '%s'\n", cgi_base->executable.buf);
	}
#endif

	/* The last one
	 */
	*last_header_offset = hdl->write_buffer.len;

	set_env (cgi_base, "SCRIPT_FILENAME", cgi_base->executable.buf, cgi_base->executable.len);
	TRACE (ENTRIES, "SCRIPT_FILENAME '%s'\n", cgi_base->executable.buf);

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
add_empty_packet (cherokee_handler_fcgi_t *hdl, cuint_t type)
{
	FCGI_BeginRequestRecord  request;
  
	fcgi_build_header (&request.header, type, 1, 0, 0);
	cherokee_buffer_add (&hdl->write_buffer, (void *)&request.header, sizeof(FCGI_Header));

	TRACE (ENTRIES, "empty packet type=%d, len=%d\n", type, hdl->write_buffer.len);
}


static ret_t
build_header (cherokee_handler_fcgi_t *hdl, cherokee_buffer_t *buffer)
{
	FCGI_BeginRequestRecord  request;
	cuint_t                  last_header_offset;
	cherokee_connection_t   *conn                = HANDLER_CONN(hdl);

	/* Take care here, if the connection is reinjected, it
	 * shouldn't parse the arguments again.
	 */
	if (conn->arguments == NULL)
		cherokee_connection_parse_args (conn);

	cherokee_buffer_clean (buffer);

	/* FCGI_BEGIN_REQUEST
	 */	
	fcgi_build_header (&request.header, FCGI_BEGIN_REQUEST, 1, sizeof(request.body), 0);
	fcgi_build_request_body (&request);
	
	cherokee_buffer_add (buffer, (void *)&request, sizeof(FCGI_BeginRequestRecord));
	TRACE (ENTRIES, "Added FCGI_BEGIN_REQUEST, len=%d\n", buffer->len);

	/* Add enviroment variables
	 */
	cherokee_handler_cgi_base_build_envp (HDL_CGI_BASE(hdl), conn);

	add_extra_fcgi_env (hdl, &last_header_offset);
	fixup_padding (buffer, last_header_offset);

	/* There are no more parameters
	 */
	add_empty_packet (hdl, FCGI_PARAMS);

	TRACE (ENTRIES, "Added FCGI_PARAMS, len=%d\n", buffer->len);
	return ret_ok;
}


static ret_t 
connect_to_server (cherokee_handler_fcgi_t *hdl)
{
	ret_t                          ret;
	cherokee_connection_t         *conn  = HANDLER_CONN(hdl);
	cherokee_handler_fcgi_props_t *props = HANDLER_FCGI_PROPS(hdl);

	/* Get a reference to the target host
	 */
	if (hdl->src_ref == NULL) {
		ret = cherokee_balancer_dispatch (props->balancer, conn, &hdl->src_ref);
		if (ret != ret_ok)
			return ret;
	}

	/* Try to connect
	 */
	if (hdl->src_ref->type == source_host)
		return cherokee_source_connect_polling (hdl->src_ref, &hdl->socket, conn);		

	return cherokee_source_interpreter_connect_polling (SOURCE_INT(hdl->src_ref),
							    &hdl->socket, conn, 
							    &hdl->spawned);
}


static ret_t 
do_send (cherokee_handler_fcgi_t *hdl, cherokee_buffer_t *buffer)
{
	ret_t                  ret;
	size_t                 written = 0;
	cherokee_connection_t *conn    = HANDLER_CONN(hdl);
	
	ret = cherokee_socket_bufwrite (&hdl->socket, buffer, &written);
	switch (ret) {
	case ret_ok:
		break;
	case ret_eagain:
		if (written > 0)
			break;
		return ret_eagain;
	default:
		conn->error_code = http_bad_gateway;
		return ret_error;
	}
	
	cherokee_buffer_move_to_begin (buffer, written);

	TRACE (ENTRIES, "sent remaining=%d\n", buffer->len);

	if (! cherokee_buffer_is_empty (buffer))
		return ret_eagain;
	
	return ret_ok;
}


static ret_t
send_no_post (cherokee_handler_fcgi_t *hdl, cherokee_buffer_t *buf)
{
	switch (hdl->post_phase) {
	case fcgi_post_init:
		add_empty_packet (hdl, FCGI_STDIN);
		hdl->post_phase = fcgi_post_write;

	case fcgi_post_write:
		return do_send (hdl, buf);

	default:
		SHOULDNT_HAPPEN;
	}
	return ret_error;
}


static ret_t
send_post (cherokee_handler_fcgi_t *hdl, cherokee_buffer_t *buf)
{
	ret_t                    ret;
	cherokee_connection_t   *conn         = HANDLER_CONN(hdl);
	static FCGI_Header       empty_header = {0,0,0,0,0,0,0,0};
			
	switch (hdl->post_phase) {
	case fcgi_post_init:
		TRACE (ENTRIES, "Post %s\n", "init");

		/* Init the POST storing object
		 */
		cherokee_post_walk_reset (&conn->post);
		cherokee_post_get_len (&conn->post, &hdl->post_len);

		if (hdl->post_len <= 0)
			return ret_ok;
		
		hdl->post_phase = fcgi_post_read;

	case fcgi_post_read:
		TRACE (ENTRIES, "Post %s\n", "read");

		/* Add space for the header, it'll filled out later on..
		 */
		if (cherokee_buffer_is_empty (buf)) {
			cherokee_buffer_add (buf, (const char *)&empty_header, sizeof (FCGI_Header));
		}

		/* Take a chunck of post
		 */
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

		TRACE (ENTRIES, "Post buffer.len %d\n", buf->len);

		/* Complete the header
		 */
		if (buf->len > sizeof(FCGI_Header)) {
			fcgi_build_header ((FCGI_Header *)buf->buf, FCGI_STDIN, 1,
					   buf->len - sizeof(FCGI_Header), 0);
		}

		/* Close STDIN if it was the last chunck
		 */
		ret = cherokee_post_walk_finished (&conn->post);
		if (ret == ret_ok) {
			add_empty_packet (hdl, FCGI_STDIN);
		}

		hdl->post_phase = fcgi_post_write;

	case fcgi_post_write:
		TRACE (ENTRIES, "Post write, buf.len=%d (header len %d)\n", buf->len, sizeof(FCGI_Header));

		if (! cherokee_buffer_is_empty (buf)) {
			ret = do_send (hdl, buf);
			switch (ret) {
                        case ret_ok:
                                break;
                        case ret_eagain:
                                return ret_eagain;
                        case ret_eof:
                        case ret_error:
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

		TRACE (ENTRIES, "Post %s\n", "finished");
		return ret_ok;

	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}


ret_t 
cherokee_handler_fcgi_init (cherokee_handler_fcgi_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);
	
	switch (HDL_CGI_BASE(hdl)->init_phase) {
	case hcgi_phase_build_headers:
		TRACE (ENTRIES, "Init: %s\n", "begins");

		/* Prepare Post
		 */
		if (! cherokee_post_is_empty (&conn->post)) {
			cherokee_post_walk_reset (&conn->post);
			cherokee_post_get_len (&conn->post, &hdl->post_len);
		}

		/* Extracts PATH_INFO and filename from request uri 
		 */		
		ret = cherokee_handler_cgi_base_extract_path (HDL_CGI_BASE(hdl), false);
		if (unlikely (ret < ret_ok)) return ret; 

		/* Build the headers
		 */
		ret = build_header (hdl, &hdl->write_buffer);
		if (unlikely (ret != ret_ok)) return ret;

		HDL_CGI_BASE(hdl)->init_phase = hcgi_phase_connect;

	case hcgi_phase_connect:
		TRACE (ENTRIES, "Init: %s\n", "connect");

 		/* Connect	
		 */
		ret = connect_to_server (hdl);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			return ret_eagain;
		case ret_deny:
			conn->error_code = http_gateway_timeout;
			return ret_error;
		default:
			conn->error_code = http_service_unavailable;
			return ret_error;
		}

		HDL_CGI_BASE(hdl)->init_phase = hcgi_phase_send_headers;

	case hcgi_phase_send_headers:
		TRACE (ENTRIES, "Init: %s\n", "send_headers");

		/* Send the header
		 */
		ret = do_send (hdl, &hdl->write_buffer);
		if (ret != ret_ok)
			return ret;

		HDL_CGI_BASE(hdl)->init_phase = hcgi_phase_send_post;

	case hcgi_phase_send_post:
		/* Send the Post
		 */
		if (hdl->post_len > 0) {
			return send_post (hdl, &hdl->write_buffer);
		} else {
			ret = send_no_post (hdl, &hdl->write_buffer);
			if (ret != ret_ok)
				return ret;
		}
		break;
	}

	TRACE (ENTRIES, "Init %s\n", "finishes");
		
	cherokee_buffer_clean (&hdl->write_buffer);
	return ret_ok;
}

