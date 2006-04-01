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
#include "downloader.h"
#include "downloader-protected.h"
#include "request.h"
#include "url.h"
#include "header-protected.h"
#include "util.h"

#include <errno.h>

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>     /* defines FIONBIO and FIONREAD */
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>    /* defines SIOCATMARK */
#endif


ret_t 
cherokee_downloader_new (cherokee_downloader_t **downloader)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n, downloader);

	/* Init
	 */
	ret = cherokee_downloader_init (n);
	if (unlikely(ret != ret_ok)) return ret;

	/* Return the object
	 */
	*downloader = n;
	return ret_ok;
}


ret_t 
cherokee_downloader_init (cherokee_downloader_t *n)
{
	int   i;
	ret_t ret;

	/* Build
	 */
	ret = cherokee_request_header_init (&n->request);
	if (unlikely(ret != ret_ok)) return ret;

	ret = cherokee_buffer_init (&n->request_header);
	if (unlikely(ret != ret_ok)) return ret;

	ret = cherokee_buffer_init (&n->reply_header);
	if (unlikely(ret != ret_ok)) return ret;	

	ret = cherokee_buffer_init (&n->body);
	if (unlikely(ret != ret_ok)) return ret;		

	ret = cherokee_socket_new (&n->socket);
	if (unlikely(ret != ret_ok)) return ret;	

	ret = cherokee_header_new (&n->header);	
	if (unlikely(ret != ret_ok)) return ret;

	/* Events
	 */
	n->callback.init        = NULL;
	n->callback.has_headers = NULL;
	n->callback.read_body   = NULL;
	n->callback.finish      = NULL;

	for (i=0; i < downloader_event_NUMBER; i++) {
		n->callback.param[i] = NULL;
	}


	/* Init the properties
	 */
	n->fdpoll   = NULL;
	n->phase    = downloader_phase_init;

	n->post_ref  = NULL;
	n->post_sent = 0;

	/* Lengths
	 */
	n->content_length = -1;

	/* Info
	 */
	n->info.headers_sent = 0;
	n->info.headers_recv = 0;
	n->info.post_sent    = 0;
	n->info.body_recv    = 0;

	return ret_ok;
}


ret_t 
cherokee_downloader_free (cherokee_downloader_t *downloader)
{
	cherokee_downloader_mrproper (downloader);

	free (downloader);
	return ret_ok;
}


ret_t 
cherokee_downloader_mrproper (cherokee_downloader_t *downloader)
{
	/* Remove the socket from the poll
	 */
	cherokee_fdpoll_del (downloader->fdpoll, SOCKET_FD(downloader->socket));
	
	/* Free the memory
	 */
	cherokee_request_header_mrproper (&downloader->request);
	cherokee_buffer_mrproper (&downloader->request_header);
	cherokee_buffer_mrproper (&downloader->reply_header);
	cherokee_buffer_mrproper (&downloader->body);
	cherokee_socket_free (downloader->socket);
	cherokee_header_free (downloader->header);

	return ret_ok;
}


ret_t 
cherokee_downloader_set_fdpoll (cherokee_downloader_t *downloader, cherokee_fdpoll_t *fdpoll)
{
	if (downloader->fdpoll != NULL) {
		PRINT_ERROR_S ("ERROR: fdpoll already set\n");
	}

	downloader->fdpoll = fdpoll;
	return ret_ok;
}


ret_t 
cherokee_downloader_set_url (cherokee_downloader_t *downloader, cherokee_buffer_t *url_string)
{
	ret_t                      ret;
	cherokee_request_header_t *req = &downloader->request;

	/* Parse the string in a URL object
	 */
	ret = cherokee_url_parse (&req->url, url_string);
	if (unlikely(ret < ret_ok)) return ret;
	
	return ret_ok;
}


ret_t 
cherokee_downloader_set_keepalive (cherokee_downloader_t *downloader, cherokee_boolean_t active)
{
	downloader->request.keepalive = active;
	return ret_ok;
}


ret_t 
cherokee_downloader_get_reply_code (cherokee_downloader_t *downloader, cherokee_http_t *code)
{
	*code = downloader->header->response;
	return ret_ok;
}


ret_t 
cherokee_downloader_connect (cherokee_downloader_t *downloader)
{
	int                 r;
	ret_t               ret;
	cherokee_socket_t  *sock = downloader->socket;
	cherokee_url_t     *url  = &downloader->request.url;

	/* Create the socket
	 */
	ret = cherokee_socket_set_client (sock, AF_INET);
	if (unlikely(ret != ret_ok)) return ret_error;
	
	/* Set the port
	 */
	SOCKET_SIN_PORT(sock) = htons(url->port);

	/* Find the IP from the hostname
	 * Maybe it is a IP..
	 */
	ret = cherokee_socket_pton (sock, &url->host);
	if (ret != ret_ok) {

		/* Ops! no, it could be a hostname..
		 * Try to resolv it!
		 */
 		ret = cherokee_socket_gethostbyname (sock, &url->host);
		if (unlikely(ret != ret_ok)) return ret_error;
	}

	/* Connect to server
	 */
	ret = cherokee_socket_connect (sock);
	if (unlikely(ret != ret_ok)) return ret;

	/* Enables nonblocking I/O.
	 */
	cherokee_fd_set_nonblocking (SOCKET_FD(sock));

	/* Add the socket to the file descriptors poll
	 */
	ret = cherokee_fdpoll_add (downloader->fdpoll, SOCKET_FD(sock), 1);
	if (ret > ret_ok) {
		PRINT_ERROR ("Can not add file descriptor (%d) to fdpoll\n", r);
		return ret;
	}

	/* Maybe execute the callback
	 */
	if (downloader->callback.init != NULL) {
		ret = (* downloader->callback.init) (downloader, downloader->callback.param[downloader_event_init]);
	}

	/* Is this connection TLS?
	 */
	if (url->protocol == https) {
		ret = cherokee_socket_init_client_tls (sock);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}


static int
is_connected (cherokee_downloader_t *downloader)
{
	return (downloader->socket->socket != -1);
}


static ret_t
downloader_send_buffer (cherokee_downloader_t *downloader, cherokee_buffer_t *buf)
{
	ret_t              ret;
	cherokee_socket_t *sock;
	size_t             written = 0;

	sock = downloader->socket;
	
	ret = cherokee_socket_write (sock, buf, &written);
	switch (ret) {
	case ret_ok:
		/* Drop the header that has been sent
		 */
		cherokee_buffer_drop_endding (buf, written);
		if (cherokee_buffer_is_empty (buf)) {
			return ret_ok;
		}

		return ret_eagain;

	case ret_eagain: 
		return ret;

	default:
		return ret_error;
	}
}


static ret_t
downloader_header_read (cherokee_downloader_t *downloader)
{
	ret_t               ret;
	size_t              readed = 0;
	cherokee_socket_t  *sock;

	sock = downloader->socket;
	
	ret = cherokee_socket_read (sock, &downloader->reply_header, DEFAULT_RECV_SIZE, &readed);
	switch (ret) {
	case ret_eof:     
		return ret_eof;

	case ret_eagain:  
		return ret_eagain;

	case ret_ok: 
	{
		uint32_t len;

		/* The socket seems to be closed
		 */
		if (readed == 0) {
			return ret_eof;
		}

		/* Count
		 */
		downloader->info.headers_recv += readed;

		/* Check the header. Is it complete? 
		 */
		ret = cherokee_header_has_header (downloader->header, &downloader->reply_header, readed+4);
		if (ret != ret_ok) {
			/* It needs to read more header..
			 */
			return ret_eagain;
		}
		
		/* Parse the header
		 */
		ret = cherokee_header_parse (downloader->header,
					     &downloader->reply_header,
					     header_type_response);		
		if (unlikely(ret != ret_ok)) return ret_error;

		/* Look for the length, it will need to drop out the header from the buffer
		 */
		cherokee_header_get_length (downloader->header, &len);

		/* Maybe it has some body
		 */
		if (downloader->reply_header.len > len) {
			uint32_t body_chunk;
			
			body_chunk = downloader->reply_header.len - len;
			
			downloader->info.body_recv += body_chunk;
			cherokee_buffer_add (&downloader->body, downloader->reply_header.buf + len, body_chunk);

			cherokee_buffer_drop_endding (&downloader->reply_header, body_chunk);
		}

		/* Try to read the "Content-length" response header
		 */
		ret = cherokee_header_has_known (downloader->header, header_content_length);
		if (ret == ret_ok) {
			cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

			ret = cherokee_header_copy_known (downloader->header, header_content_length, &tmp);
			downloader->content_length = atoi (tmp.buf);

			cherokee_buffer_mrproper (&tmp);
		}

		/* It has the parsed header..
		 * execute the callback function
		 */
		if (downloader->callback.has_headers != NULL) {
			ret = (* downloader->callback.has_headers) (downloader, downloader->callback.param[downloader_event_has_headers]);
		}

		/* Check if it was a successful connection
		 */
		if (!http_type_200(downloader->header->response)) {
			return ret_error;
		}

		return ret_ok;
	}

	case ret_error:
		/* Opsss.. something has failed
		 */
		return ret_error;

	default:
		PRINT_ERROR ("Unknown ret code %d\n", ret);
		SHOULDNT_HAPPEN;
		return ret;
	}
	
	return ret_error;
}


static ret_t
downloader_step (cherokee_downloader_t *downloader)
{
	ret_t               ret;
	size_t              readed = 0;
	cherokee_socket_t  *sock;

	sock = downloader->socket;

	ret = cherokee_socket_read (sock, &downloader->body, DEFAULT_RECV_SIZE, &readed);
	switch (ret) {
	case ret_eagain:
		return ret_eagain;

	case ret_eof:
		goto finish_eof;

	case ret_ok:
		/* The connection cloud be closed
		 */
		if (readed == 0) goto finish_eof;
		
		/* Count
		 */
		downloader->info.body_recv += readed;

		/* There are new body. Execute the callback!
		 */
		if (downloader->callback.read_body != NULL) {
			ret = (* downloader->callback.read_body) (downloader, downloader->callback.param[downloader_event_read_body]);
		}

		/* Has it finished?
		 */
		if (downloader->info.body_recv >= downloader->content_length) {
			goto finish_eof;
		}

		break;
	default:
		return ret;
	}
	
	return ret_ok;

finish_eof:
	if (downloader->callback.finish != NULL) {
		ret = (* downloader->callback.finish) (downloader, downloader->callback.param[downloader_event_finish]);
	}

	return ret_eof;	
}


ret_t 
cherokee_downloader_step (cherokee_downloader_t *downloader)
{
	ret_t ret;

	switch (downloader->phase) {
	case downloader_phase_init: {
		cherokee_request_header_t *req = &downloader->request;

		/* Maybe add the post info
		 */
		if (downloader->post_ref != NULL) {
			req->method   = http_post;
			req->post_len = downloader->post_ref->len;
		}

		/* Build the request header
		 */
		ret = cherokee_request_header_build_string (req, &downloader->request_header);
		if (unlikely(ret < ret_ok)) return ret;

		/* Deal with the connection
		 */
		if (! is_connected (downloader)) {
			ret = cherokee_downloader_connect (downloader);
			if (ret < ret_ok) return ret;
		}

		/* Everything is ok, go ahead!
		 */
		downloader->phase = downloader_phase_send_headers;
	}
	case downloader_phase_send_headers:
		/* Can it write in the socket?
		 */
		if (! cherokee_fdpoll_check (downloader->fdpoll, SOCKET_FD(downloader->socket), 1)) {
			/* Try later..
			 */
			return ret_eagain;
		}

		ret = downloader_send_buffer (downloader, &downloader->request_header);
		if (unlikely(ret != ret_ok)) return ret;

		/* Come on..
		 */
		downloader->phase = downloader_phase_send_post;

	case downloader_phase_send_post:
		if (downloader->post_ref != NULL) {
			/* Can it write in the socket?
			 */
			if (! cherokee_fdpoll_check (downloader->fdpoll, SOCKET_FD(downloader->socket), 1)) {
				return ret_eagain;
			}

			ret = downloader_send_buffer (downloader, downloader->post_ref);
			if (unlikely(ret != ret_ok)) return ret;
		}

		/* Switch the socket to read-only mode..
		 */
		cherokee_fdpoll_set_mode (downloader->fdpoll, SOCKET_FD(downloader->socket), 0);

		downloader->phase = downloader_phase_read_headers;
		break;

	case downloader_phase_read_headers:
		/* Can it read in the socket?
		 */
		if (! cherokee_fdpoll_check (downloader->fdpoll, SOCKET_FD(downloader->socket), 0)) {
			/* Try later..
			 */
			return ret_eagain;
		}

		ret = downloader_header_read (downloader);
		if (unlikely(ret != ret_ok)) return ret;

		/* We have the header parsed, continue..
		 */
		downloader->phase = downloader_phase_step;

		/* Does it read the full reply in the first received chunk?
		 */
		if (downloader->info.body_recv >= downloader->content_length) {
			if (downloader->callback.finish != NULL) {
				(* downloader->callback.finish) (downloader, downloader->callback.param[downloader_event_finish]);
			}
			return ret_eof;
		}

	case downloader_phase_step:
		/* Can it read in the socket?
		 */
		if (! cherokee_fdpoll_check (downloader->fdpoll, SOCKET_FD(downloader->socket), 0)) {
			/* Try later..
			 */
			return ret_eagain;
		}

		ret = downloader_step (downloader);

		switch (ret) {
		case ret_ok:
			/* It goes ok
			 */
			return ret_ok;

		case ret_eagain:
			/* One moment, please..
			 */
			return ret_eagain;

		case ret_eof:
			/* Finish!
			 */
			return ret_eof;

		case ret_error:
			/* Opsss..
			 */
			return ret_error;

		default:
			RET_UNKNOWN(ret);
			return ret;
		}

	default:
		SHOULDNT_HAPPEN;
		break;
	}

	return ret_ok;
}



ret_t 
cherokee_downloader_connect_event (cherokee_downloader_t *downloader, cherokee_downloader_event_t event, void *func, void *param)
{
	/* Set the parameter
	 */
	downloader->callback.param[event] = param;
	
	/* Set the function
	 */
	switch (event) {
	case downloader_event_init:
		downloader->callback.init = (cherokee_downloader_init_t) func;
		break;

	case downloader_event_has_headers:
		downloader->callback.has_headers = (cherokee_downloader_has_headers_t) func;
		break;

	case downloader_event_read_body:
		downloader->callback.read_body = (cherokee_downloader_read_body_t) func;
		break;

	case downloader_event_finish:
		downloader->callback.finish = (cherokee_downloader_finish_t) func;
		break;

	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}
	
	return ret_ok;
}


ret_t 
cherokee_downloader_post_set (cherokee_downloader_t *downloader, cherokee_buffer_t *post)
{
	if (downloader->post_ref != NULL) {
		PRINT_ERROR_S ("WARNING: Overwriting post info\n");
	}

	downloader->post_sent = 0;
	downloader->post_ref  = post;

	return ret_ok;
}


ret_t 
cherokee_downloader_reuse (cherokee_downloader_t *downloader)
{
	cherokee_fdpoll_set_mode (downloader->fdpoll, SOCKET_FD(downloader->socket), 1);

	downloader->phase = downloader_phase_init;

	downloader->post_ref  = NULL;
	downloader->post_sent = 0;

	cherokee_buffer_clean (&downloader->request_header);
	cherokee_buffer_clean (&downloader->reply_header);
	cherokee_buffer_clean (&downloader->body);

	cherokee_request_header_clean (&downloader->request);
	return ret_ok;
}

