/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2007 Alvaro Lopez Ortega
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

#define ENTRIES "downloader"


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

	ret = cherokee_socket_init (&n->socket);
	if (unlikely(ret != ret_ok)) return ret;	

	ret = cherokee_header_new (&n->header);	
	if (unlikely(ret != ret_ok)) return ret;

	cherokee_buffer_init (&n->tmp1);
	cherokee_buffer_init (&n->tmp2);

	/* Init the properties
	 */
	n->phase             = downloader_phase_init;
	n->post              = NULL;

	/* Lengths
	 */
	n->content_length    = -1;

	/* Info
	 */
	n->info.request_sent = 0;
	n->info.headers_recv = 0;
	n->info.post_sent    = 0;
	n->info.body_recv    = 0;

	n->status = downloader_status_none;

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
	/* Free the memory
	 */
	cherokee_request_header_mrproper (&downloader->request);

	cherokee_buffer_mrproper (&downloader->request_header);
	cherokee_buffer_mrproper (&downloader->reply_header);
	cherokee_buffer_mrproper (&downloader->body);

	cherokee_buffer_mrproper (&downloader->tmp1);
	cherokee_buffer_mrproper (&downloader->tmp2);

	cherokee_socket_close (&downloader->socket);
	cherokee_socket_mrproper (&downloader->socket);

	cherokee_header_free (downloader->header);

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
	ret_t               ret;
	cherokee_socket_t  *sock = &downloader->socket;
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
	return (downloader->socket.socket != -1);
}


static ret_t
downloader_send_buffer (cherokee_downloader_t *downloader, cherokee_buffer_t *buf)
{
	ret_t              ret;
	size_t             written = 0;
	cherokee_socket_t *sock    = &downloader->socket;;

	ret = cherokee_socket_bufwrite (sock, buf, &written);
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
downloader_header_read (cherokee_downloader_t *downloader, cherokee_buffer_t *tmp1, cherokee_buffer_t *tmp2)
{
	ret_t               ret;
	cuint_t             len;
	size_t              readed = 0;
	cherokee_socket_t  *sock   = &downloader->socket;

	ret = cherokee_socket_bufread (sock, &downloader->reply_header, DEFAULT_RECV_SIZE, &readed);
	switch (ret) {
	case ret_eof:     
		return ret_eof;

	case ret_eagain:  
		return ret_eagain;

	case ret_ok: 
		/* Count
		 */
		downloader->info.headers_recv += readed;

		/* Check the header. Is it complete? 
		 */
		ret = cherokee_header_has_header (downloader->header, &downloader->reply_header, readed+4);
		switch (ret) {
		case ret_ok:
			break;
		case ret_not_found:
			/* It needs to read more headers ... 
			 */
			return ret_eagain;
		default:
			/* Too many initial CRLF 
			 */
			return ret_error;
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
			cherokee_buffer_clean (tmp1);
			ret = cherokee_header_copy_known (downloader->header, header_content_length, tmp1);
			downloader->content_length = atoi (tmp1->buf);
		}

		return ret_ok;

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
	cherokee_socket_t  *sock   = &downloader->socket;

	ret = cherokee_socket_bufread (sock, &downloader->body, DEFAULT_RECV_SIZE, &readed);
	switch (ret) {
	case ret_eagain:
		return ret_eagain;

	case ret_ok:
		/* Count
		 */
		downloader->info.body_recv += readed;

		/* Has it finished?
		 */
		if (downloader->info.body_recv >= downloader->content_length) {
			return ret_eof_have_data;
		}

		return ret_ok;
	default:
		return ret;
	}
	
	return ret_ok;
}


ret_t 
cherokee_downloader_step (cherokee_downloader_t *downloader, cherokee_buffer_t *ext_tmp1, cherokee_buffer_t *ext_tmp2)
{
	ret_t              ret;
	cherokee_buffer_t *tmp1;
	cherokee_buffer_t *tmp2;

	/* Set the temporary buffers
	 */
	tmp1 = (ext_tmp1) ? ext_tmp1 : &downloader->tmp1;
	tmp2 = (ext_tmp2) ? ext_tmp2 : &downloader->tmp2;

	/* Process it
	 */
	switch (downloader->phase) {
	case downloader_phase_init: {
		cherokee_request_header_t *req = &downloader->request;

		TRACE(ENTRIES, "Phase %s\n", "init");
		
		/* Maybe add the post info
		 */
		if (downloader->post != NULL) {
			req->method = http_post;
			cherokee_post_walk_reset (downloader->post);
		}

		/* Build the request header
		 */
		ret = cherokee_request_header_build_string (req, &downloader->request_header, tmp1, tmp2);
		if (unlikely(ret < ret_ok))
			return ret;

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
		TRACE(ENTRIES, "Phase %s\n", "send_headers");

		ret = downloader_send_buffer (downloader, &downloader->request_header);
		if (unlikely(ret != ret_ok)) return ret;

		downloader->status = downloader_status_headers_sent;
		downloader->phase = downloader_phase_send_post;

	case downloader_phase_send_post:
		TRACE(ENTRIES, "Phase %s\n", "send_post");

		if (downloader->post != NULL) {
			ret = cherokee_post_walk_to_fd (downloader->post, downloader->socket.socket, NULL, NULL);

/*			ret = send_post (downloader); */
/* 			ret = downloader_send_buffer (downloader, downloader->post_ref); */
			if (unlikely(ret != ret_ok)) return ret;
		}

		downloader->status = downloader->status | downloader_status_post_sent;
		downloader->phase = downloader_phase_read_headers;
		break;

	case downloader_phase_read_headers:
		TRACE(ENTRIES, "Phase %s\n", "read_headers");

		ret = downloader_header_read (downloader, tmp1, tmp2);
		if (unlikely(ret != ret_ok)) return ret;

		/* We have the header parsed, continue..
		 */
		downloader->status = downloader->status | downloader_status_headers_received;
		downloader->phase = downloader_phase_step;

		/* Does it read the full reply in the first received chunk?
		 */
		if (downloader->info.body_recv >= downloader->content_length) {
			downloader->status = downloader->status | downloader_status_data_available | downloader_status_finished;
			return ret_eof_have_data;
		}

	case downloader_phase_step:
		TRACE(ENTRIES, "Phase %s\n", "step");

		ret = downloader_step (downloader);
		switch (ret) {
		case ret_error:
			break;
		case ret_ok:
			downloader->status = downloader->status | downloader_status_data_available;
			break;
		case ret_eof_have_data:
			downloader->status = downloader->status | downloader_status_data_available | downloader_status_finished;
			break;
		case ret_eof:
			downloader->status = downloader->status & (~downloader_status_data_available | downloader_status_finished);
			break;
		case ret_eagain:
			downloader->status = downloader->status & ~downloader_status_data_available;
			break;
		default:
			RET_UNKNOWN(ret);
		}
		return ret;

	case downloader_phase_finished:
		TRACE(ENTRIES, "Phase %s\n", "finished");

		downloader->status = downloader->status & ~downloader_status_data_available & downloader_status_finished;
		return ret_ok;

	default:
		SHOULDNT_HAPPEN;
		break;
	}

	return ret_ok;
}


ret_t 
cherokee_downloader_post_set (cherokee_downloader_t *downloader, cherokee_post_t *post)
{
	if (downloader->post != NULL) {
		PRINT_ERROR_S ("WARNING: Overwriting post info\n");
	}

	downloader->post = post;
	return ret_ok;
}


ret_t 
cherokee_downloader_reuse (cherokee_downloader_t *downloader)
{
	downloader->phase = downloader_phase_init;
	downloader->post  = NULL;

	cherokee_buffer_clean (&downloader->request_header);
	cherokee_buffer_clean (&downloader->reply_header);
	cherokee_buffer_clean (&downloader->body);

	cherokee_request_header_clean (&downloader->request);
	return ret_ok;
}


ret_t 
cherokee_downloader_is_request_sent (cherokee_downloader_t *downloader)
{
	if (downloader->phase > downloader_phase_send_headers) 
		return ret_ok;

	return ret_deny;
}

ret_t 
cherokee_downloader_headers_available(cherokee_downloader_t *downloader)
{
	return (downloader->phase == downloader_phase_step) ? ret_ok: ret_deny;
}
ret_t 
cherokee_downloader_data_available(cherokee_downloader_t *downloader)
{
	return (downloader->phase == downloader_phase_step) ? ret_ok: ret_deny;
}

ret_t
cherokee_downloader_finished(cherokee_downloader_t *downloader) 
{
	ret_t ret = ret_deny;

	if (downloader->info.body_recv >= downloader->content_length) {
		ret = ret_ok;
	} else 	if (downloader->phase == downloader_phase_finished) {
		ret = ret_ok;
	}
	return ret;
}

ret_t
cherokee_downloader_get_status(cherokee_downloader_t *downloader, cherokee_downloader_status_t *status)
{
	if (status != NULL) {
		*status = downloader->status;
	}

	return ret_ok;
}
