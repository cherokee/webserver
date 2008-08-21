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

	ret = cherokee_header_new (&n->header, header_type_response);	
	if (unlikely(ret != ret_ok)) return ret;

	cherokee_buffer_init (&n->proxy);
	n->proxy_port = 0;

	cherokee_buffer_init (&n->tmp1);
	cherokee_buffer_init (&n->tmp2);

	/* Init the properties
	 */
	n->phase             = downloader_phase_init;
	n->post              = NULL;

	/* Lengths
	 */
	n->content_length    = 0;

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
cherokee_downloader_mrproper (cherokee_downloader_t *downloader)
{
	/* Free the memory
	 */
	cherokee_request_header_mrproper (&downloader->request);

	cherokee_buffer_mrproper (&downloader->request_header);
	cherokee_buffer_mrproper (&downloader->reply_header);
	cherokee_buffer_mrproper (&downloader->body);
	cherokee_buffer_mrproper (&downloader->proxy);

	cherokee_buffer_mrproper (&downloader->tmp1);
	cherokee_buffer_mrproper (&downloader->tmp2);

	cherokee_socket_close (&downloader->socket);
	cherokee_socket_mrproper (&downloader->socket);

	cherokee_header_free (downloader->header);

	return ret_ok;
}


CHEROKEE_ADD_FUNC_NEW (downloader);
CHEROKEE_ADD_FUNC_FREE (downloader);


ret_t 
cherokee_downloader_set_url (cherokee_downloader_t *downloader, cherokee_buffer_t *url_string)
{
	return cherokee_request_header_parse_string (&downloader->request, url_string);
}


ret_t 
cherokee_downloader_set_keepalive (cherokee_downloader_t *downloader, cherokee_boolean_t active)
{
	downloader->request.keepalive = active;
	return ret_ok;
}


ret_t 
cherokee_downloader_set_proxy (cherokee_downloader_t *downloader, cherokee_buffer_t *proxy, cuint_t port)
{
	char *tmp;

	/* Skip 'http(s)://'
	 */
	tmp = strchr (proxy->buf, '/');
	if (tmp == NULL) 
		tmp = proxy->buf;
	else
		tmp += 2;

	/* Copy the values
	 */
	cherokee_buffer_clean (&downloader->proxy);
	cherokee_buffer_add (&downloader->proxy, tmp, strlen(tmp));

	downloader->proxy_port = port;
	return ret_ok;
}


ret_t
cherokee_downloader_set_auth (cherokee_downloader_t *downloader, 
			      cherokee_buffer_t     *user, 
			      cherokee_buffer_t     *password)
{
	return cherokee_request_header_set_auth (&downloader->request, http_auth_basic, user, password);
}


ret_t 
cherokee_downloader_get_reply_code (cherokee_downloader_t *downloader, cherokee_http_t *code)
{
	*code = downloader->header->response;
	return ret_ok;
}


static ret_t 
connect_to (cherokee_downloader_t *downloader, cherokee_buffer_t *host, cuint_t port, int protocol)
{
	ret_t              ret;
	cherokee_socket_t *sock = &downloader->socket;

	TRACE(ENTRIES, "host=%s port=%d proto=%d\n", host->buf, port, protocol);

	/* Create the socket
	 */
	ret = cherokee_socket_set_client (sock, AF_INET);
	if (unlikely(ret != ret_ok)) return ret_error;
	
	/* Set the port
	 */
	SOCKET_SIN_PORT(sock) = htons(port);

	/* Supposing it's an IP: convert it.
	 */
	ret = cherokee_socket_pton (sock, host);
	if (ret != ret_ok) {

		/* Oops! It might be a hostname. Try to resolve it.
		 */
 		ret = cherokee_socket_gethostbyname (sock, host);
		if (unlikely(ret != ret_ok)) return ret_error;
	}

	/* Connect to server
	 */
	ret = cherokee_socket_connect (sock);
	TRACE(ENTRIES, "socket=%p ret=%d\n", sock, ret);

	if (unlikely(ret != ret_ok)) 
		return ret;

	/* Is this connection TLS?
	 */
	if (protocol == https) {
		ret = cherokee_socket_init_client_tls (sock, host);
		if (ret != ret_ok) return ret;
	}

	TRACE(ENTRIES, "Exits ok; socket=%p\n", sock);
	return ret_ok;
}


ret_t 
cherokee_downloader_connect (cherokee_downloader_t *downloader)
{
	ret_t               ret;
	cherokee_boolean_t  uses_proxy;
	cherokee_url_t     *url  = &downloader->request.url;

	/* Does it use a proxy?
	 */
	uses_proxy = ! cherokee_buffer_is_empty (&downloader->proxy);
	ret = cherokee_request_header_uses_proxy (&downloader->request, uses_proxy);
	if (ret != ret_ok) return ret;
	
	/* Connect
	 */
	if (uses_proxy) {
		ret = connect_to (downloader, &downloader->proxy, downloader->proxy_port, http);
		if (ret != ret_ok) return ret;
	} else {
		ret = connect_to (downloader, &url->host, url->port, url->protocol);
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
		cherokee_buffer_drop_ending (buf, written);
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
	size_t              read_      = 0;
	cherokee_socket_t  *sock       = &downloader->socket;
	cherokee_http_t     error_code = http_bad_request;

	ret = cherokee_socket_bufread (sock, &downloader->reply_header, DEFAULT_RECV_SIZE, &read_);
	switch (ret) {
	case ret_eof:     
		return ret_eof;

	case ret_eagain:  
		return ret_eagain;

	case ret_ok: 
		/* Count
		 */
		downloader->info.headers_recv += read_;

		/* Check the header. Is it complete? 
		 */
		ret = cherokee_header_has_header (downloader->header, &downloader->reply_header, 0);
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
		                             &error_code);
		if (unlikely (ret != ret_ok)) return ret_error;

		/* Look for the length, it will need to drop out the header from the buffer
		 */
		cherokee_header_get_length (downloader->header, &len);

		/* Maybe it has some body
		 */
		if (downloader->reply_header.len > len) {
			uint32_t body_chunk;
			
			/* Skip the CRLF separator and copy the body
			 */
			len += 2;
			body_chunk = downloader->reply_header.len - len;
			
			downloader->info.body_recv += body_chunk;
			cherokee_buffer_add (&downloader->body, downloader->reply_header.buf + len, body_chunk);

			cherokee_buffer_drop_ending (&downloader->reply_header, body_chunk);
		}

		/* Try to read the "Content-Length" response header
		 */
		ret = cherokee_header_has_known (downloader->header, header_content_length);
		if (ret == ret_ok) {
			cherokee_buffer_clean (tmp1);
			ret = cherokee_header_copy_known (downloader->header, header_content_length, tmp1);
			downloader->content_length = atoi (tmp1->buf);

			TRACE (ENTRIES, "Known lenght: %d bytes\n", downloader->content_length);
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
	size_t              read_ = 0;
	cherokee_socket_t  *sock  = &downloader->socket;

	ret = cherokee_socket_bufread (sock, &downloader->body, DEFAULT_RECV_SIZE, &read_);
	switch (ret) {
	case ret_eagain:
		return ret_eagain;

	case ret_ok:
		/* Count
		 */
		downloader->info.body_recv += read_;

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

	TRACE (ENTRIES, "phase=%d\n", downloader->phase);

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
			req->post_len = downloader->post->size;
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

		BIT_SET (downloader->status, downloader_status_headers_sent);
		downloader->phase = downloader_phase_send_post;

	case downloader_phase_send_post:
		TRACE(ENTRIES, "Phase %s\n", "send_post");

		if (downloader->post != NULL) {
			ret = cherokee_post_walk_to_fd (downloader->post, downloader->socket.socket, NULL, NULL);
/*			ret = send_post (downloader); */
/* 			ret = downloader_send_buffer (downloader, downloader->post_ref); */
			if (unlikely(ret != ret_ok)) return ret;
		}

		BIT_SET (downloader->status, downloader_status_post_sent);
		downloader->phase = downloader_phase_read_headers;
		break;

	case downloader_phase_read_headers:
		TRACE(ENTRIES, "Phase %s\n", "read_headers");

		ret = downloader_header_read (downloader, tmp1, tmp2);
		if (unlikely(ret != ret_ok)) return ret;

		/* We have the header parsed, continue..
		 */
		BIT_SET (downloader->status, downloader_status_headers_received);
		downloader->phase = downloader_phase_step;

		/* Does it read the full reply in the first received chunk?
		 */
		if (downloader->info.body_recv >= downloader->content_length) {
			BIT_SET (downloader->status, downloader_status_data_available);
			BIT_SET (downloader->status, downloader_status_finished);
			return ret_eof_have_data;
		}

	case downloader_phase_step:
		TRACE(ENTRIES, "Phase %s\n", "step");

		ret = downloader_step (downloader);
		switch (ret) {
		case ret_error:
			break;
		case ret_ok:
			BIT_SET (downloader->status, downloader_status_data_available);
			break;
		case ret_eof_have_data:
			BIT_SET (downloader->status, downloader_status_data_available);
			BIT_SET (downloader->status, downloader_status_finished);
			break;
		case ret_eof:
			BIT_UNSET (downloader->status, downloader_status_data_available);
			BIT_SET   (downloader->status, downloader_status_finished);
			break;
		case ret_eagain:
			BIT_UNSET (downloader->status, downloader_status_data_available);
			break;
		default:
			RET_UNKNOWN(ret);
		}
		return ret;

	case downloader_phase_finished:
		TRACE(ENTRIES, "Phase %s\n", "finished");

		BIT_SET   (downloader->status, downloader_status_finished);
		BIT_UNSET (downloader->status, downloader_status_data_available);
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
	TRACE(ENTRIES, "post=%p\n", post);

	if (downloader->post != NULL) {
		PRINT_ERROR_S ("WARNING: Overwriting post info\n");
	}

	downloader->post = post;
	return ret_ok;
}


ret_t 
cherokee_downloader_reuse (cherokee_downloader_t *downloader)
{
	TRACE(ENTRIES, "%p\n", downloader);

	downloader->phase = downloader_phase_init;
	downloader->post  = NULL;

	downloader->info.request_sent = 0;
	downloader->info.headers_recv = 0;
	downloader->info.post_sent    = 0;
	downloader->info.body_recv    = 0;

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


ret_t 
cherokee_downloader_get_reply_hdr (cherokee_downloader_t *downloader, cherokee_buffer_t **header)
{
	if (header != NULL) {
		*header = &downloader->reply_header;
	}

	return ret_ok;
}
