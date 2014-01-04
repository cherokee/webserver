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
#include "post.h"

#include "connection-protected.h"
#include "util.h"

#define ENTRIES           "post"
#define HTTP_100_RESPONSE "HTTP/1.1 100 Continue" CRLF CRLF


/* Base functions
 */

ret_t
cherokee_post_init (cherokee_post_t *post)
{
	post->len                = 0;
	post->has_info           = false;
	post->encoding           = post_enc_regular;
	post->read_header_phase  = cherokee_post_read_header_init;

	post->send.phase         = cherokee_post_send_phase_read;
	post->send.read          = 0;

	post->chunked.last       = false;
	post->chunked.processed  = 0;
	post->chunked.retransmit = false;

	cherokee_buffer_init (&post->send.buffer);
	cherokee_buffer_init (&post->chunked.buffer);
	cherokee_buffer_init (&post->read_header_100cont);
	cherokee_buffer_init (&post->header_surplus);
	cherokee_buffer_init (&post->progress_id);

	return ret_ok;
}

ret_t
cherokee_post_clean (cherokee_post_t *post)
{
	post->len                = 0;
	post->has_info           = false;
	post->encoding           = post_enc_regular;
	post->read_header_phase  = cherokee_post_read_header_init;

	post->send.phase         = cherokee_post_send_phase_read;
	post->send.read          = 0;

	post->chunked.last       = false;
	post->chunked.processed  = 0;
	post->chunked.retransmit = false;

	cherokee_buffer_mrproper (&post->send.buffer);
	cherokee_buffer_mrproper (&post->chunked.buffer);
	cherokee_buffer_mrproper (&post->read_header_100cont);
	cherokee_buffer_mrproper (&post->header_surplus);
	cherokee_buffer_mrproper (&post->progress_id);

	return ret_ok;
}

ret_t
cherokee_post_mrproper (cherokee_post_t *post)
{
	cherokee_buffer_mrproper (&post->send.buffer);
	cherokee_buffer_mrproper (&post->chunked.buffer);
	cherokee_buffer_mrproper (&post->read_header_100cont);
	cherokee_buffer_mrproper (&post->header_surplus);
	cherokee_buffer_mrproper (&post->progress_id);

	return ret_ok;
}


/* Methods
 */


static ret_t
parse_header (cherokee_post_t       *post,
              cherokee_connection_t *conn)
{
	ret_t    ret;
	char    *info      = NULL;
	cuint_t  info_len  = 0;
	CHEROKEE_TEMP(buf, 64);

	/* RFC 2616:
	 *
	 * If a message is received with both a Transfer-Encoding
	 * header field and a Content-Length header field, the latter
	 * MUST be ignored.
	 */

	/* Check "Transfer-Encoding"
	 */
	ret = cherokee_header_get_known (&conn->header, header_transfer_encoding, &info, &info_len);
	if (ret == ret_ok) {
		if (strncasecmp (info, "chunked", MIN(info_len, 7)) == 0) {
			TRACE (ENTRIES, "Post type: %s\n", "chunked");
			post->encoding = post_enc_chunked;
			return ret_ok;
		}
	}

	TRACE (ENTRIES, "Post type: %s\n", "plain");

	/* Check "Content-Length"
	 */
	ret = cherokee_header_get_known (&conn->header, header_content_length, &info, &info_len);
	if (unlikely (ret != ret_ok)) {
		conn->error_code = http_length_required;
		return ret_error;
	}

	/* Parse the POST length
	 */
	if (unlikely ((info == NULL)  ||
		      (info_len == 0) ||
		      (info_len >= buf_size)))
	{
		conn->error_code = http_bad_request;
		return ret_error;
	}

	memcpy (buf, info, info_len);
	buf[info_len] = '\0';

	/* Check: Post length >= 0
	 */
	post->len = (off_t) atoll(buf);
	if (unlikely (post->len < 0)) {
		conn->error_code = http_bad_request;
		return ret_error;
	}

	TRACE (ENTRIES, "Post claims to be %llu bytes long\n", post->len);
	return ret_ok;
}


static ret_t
reply_100_continue (cherokee_post_t       *post,
                    cherokee_connection_t *conn)
{
	ret_t  ret;
	size_t written;

	ret = cherokee_socket_bufwrite (&conn->socket, &post->read_header_100cont, &written);
	if (ret != ret_ok) {
		TRACE(ENTRIES, "Could not send a '100 Continue' response. Error=500.\n");
		return ret;
	}

	if (written >= post->read_header_100cont.len) {
		TRACE(ENTRIES, "Sent a '100 Continue' response.\n");
		return ret_ok;
	}

	TRACE(ENTRIES, "Sent partial '100 Continue' response: %d bytes\n", written);
	cherokee_buffer_move_to_begin (&post->read_header_100cont, written);

	return ret_eagain;
}


static ret_t
remove_surplus (cherokee_post_t       *post,
                cherokee_connection_t *conn)
{
	ret_t   ret;
	cuint_t header_len;
	cint_t  surplus_len;

	/* Get the HTTP request length
	 */
	ret = cherokee_header_get_length (&conn->header, &header_len);
	if (unlikely(ret != ret_ok)) {
		return ret;
	}

	/* Get the POST length
	 */
	if (post->len > 0) {
		/* Plain post: read what's pointed by Content-Length
		 */
		surplus_len = MIN (conn->incoming_header.len - header_len, post->len);
	} else {
		/* Chunked: Assume everything after the header is POST
		 */
		surplus_len = conn->incoming_header.len - header_len;
	}
	if (surplus_len <= 0) {
		return ret_ok;
	}

	TRACE (ENTRIES, "POST surplus: moving %d bytes\n", surplus_len);

	/* Move the surplus
	 */
	cherokee_buffer_add (&post->header_surplus,
			     conn->incoming_header.buf + header_len,
			     surplus_len);

	cherokee_buffer_remove_chunk (&conn->incoming_header,
				      header_len, surplus_len);

	TRACE (ENTRIES, "POST surplus: incoming_header is %d bytes (header len=%d)\n",
	       conn->incoming_header.len, header_len);

	return ret_ok;
}


ret_t
cherokee_post_read_header (cherokee_post_t *post,
                           void            *cnt)
{
	ret_t                  ret;
	char                  *info     = NULL;
	cuint_t                info_len = 0;
	cherokee_connection_t *conn     = CONN(cnt);

	switch (post->read_header_phase) {
	case cherokee_post_read_header_init:
		/* Read the header
		 */
		ret = parse_header (post, conn);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}

		post->has_info = true;

		ret = remove_surplus (post, conn);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}

		/* Expect: 100-continue
		 * http://www.w3.org/Protocols/rfc2616/rfc2616-sec8.html
		 */
		ret = cherokee_header_get_known (&conn->header, header_expect, &info, &info_len);
		if (likely (ret != ret_ok)) {
			return ret_ok;
		}

		cherokee_buffer_add_str (&post->read_header_100cont, HTTP_100_RESPONSE);
		post->read_header_phase = cherokee_post_read_header_100cont;

	case cherokee_post_read_header_100cont:
		return reply_100_continue (post, conn);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


static ret_t
process_chunk (cherokee_post_t   *post,
               cherokee_buffer_t *in,
               cherokee_buffer_t *out)
{
	char    *p;
	char    *begin;
	char    *end;
	size_t  content_size;

	TRACE (ENTRIES, "Post in-buffer len=%d\n", in->len);

	p     = in->buf;
	begin = in->buf;

	while (true) {
		end = in->buf + in->len;

		/* Iterate through the number
		 */
		while ((p < end) &&
				(((*p >= '0') && (*p <= '9')) ||
				((*p >= 'a') && (*p <= 'f')) ||
				((*p >= 'A') && (*p <= 'F')))) {
			p++;
		}

		if (unlikely (p+2 > end)) {
			return ret_ok;
		}

		/* Check the CRLF after the length
		*/
		if (p[0] != CHR_CR) {
			return ret_error;
		}
		if (p[1] != CHR_LF) {
			return ret_error;
		}

		p += 2;

		/* Read the length
		*/
		content_size = (size_t) strtoul (begin, NULL, 16);
		if (unlikely (errno != 0)) {
			return ret_error;
		}

		/* Check if there's enough info
		*/
		if (content_size == 0) {
			post->chunked.last = true;

		} else if (p + content_size + 2 > end) {
			TRACE (ENTRIES, "Unfinished chunk(len="FMT_OFFSET"), has=%d, out->len="FMT_OFFSET"\n",
			                content_size, (int)(end-p), out->len);
			break;
		}

		/* Last block check
		*/
		if (post->chunked.last) {
			TRACE(ENTRIES, "Last chunk: %s\n", "exiting");

			if (post->chunked.retransmit) {
				cherokee_buffer_add_str (out, "0" CRLF);
			}
			begin = p;
			break;
		}

		/* Move the information
		 */
		if (post->chunked.retransmit) {
			cherokee_buffer_add (out, begin, (p + content_size + 2) - begin);
		} else {
			cherokee_buffer_add (out, p, content_size);
		}

		TRACE (ENTRIES, "Processing chunk len=%d\n", content_size);

		/* Next iteration
		 */
		begin = p + content_size + 2;
		p = begin;
	}

	/* Clean up in-buffer
	 */
	if (begin != in->buf) {
		cherokee_buffer_move_to_begin (in, begin - in->buf);
	}

	/* Very unlikely, but still possible
	 */
	if (! cherokee_buffer_is_empty(in)) {
		TRACE (ENTRIES, "There are %d left-over bytes in the post buffer -> incoming header", in->len);
#if 0
		cherokee_buffer_add_buffer (&conn->incoming_header, in);
		cherokee_buffer_clean (in);
#endif
	}

	TRACE (ENTRIES, "Un-chunked buffer len=%d\n", out->len);
	return ret_ok;
}


static ret_t
do_read_plain (cherokee_post_t   *post,
               cherokee_socket_t *sock_in,
               cherokee_buffer_t *buffer,
               off_t              to_read)
{
	ret_t  ret;
	size_t len;

	/* Surplus from header read
	 */
	if (! cherokee_buffer_is_empty (&post->header_surplus)) {
		TRACE (ENTRIES, "Post appending %d surplus bytes\n", post->header_surplus.len);
		post->send.read += post->header_surplus.len;

		cherokee_buffer_add_buffer (buffer, &post->header_surplus);
		cherokee_buffer_clean (&post->header_surplus);

		return ret_ok;
	}

	/* Read
	 */
	if (to_read <= 0) {
		return ret_ok;
	}

	TRACE (ENTRIES, "Post reading from client (to_read=%d)\n", to_read);
	ret = cherokee_socket_bufread (sock_in, buffer, to_read, &len);
	TRACE (ENTRIES, "Post read from client: ret=%d len=%d\n", ret, len);

	if (ret != ret_ok) {
		return ret;
	}

	post->send.read += len;
	return ret_ok;
}


static ret_t
do_read_chunked (cherokee_post_t   *post,
                 cherokee_socket_t *sock_in,
                 cherokee_buffer_t *buffer)
{
	ret_t ret;

	/* Try to read
	 */
	ret = do_read_plain (post, sock_in, &post->chunked.buffer, POST_READ_SIZE);
	switch(ret) {
	case ret_ok:
		break;
	case ret_eagain:
		return ret_eagain;
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	/* Process the buffer
	 */
	ret = process_chunk (post, &post->chunked.buffer, buffer);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	if (post->chunked.last) {
		cherokee_buffer_mrproper (&post->chunked.buffer);
	}

	return ret_ok;
}


ret_t
cherokee_post_read (cherokee_post_t   *post,
                    cherokee_socket_t *sock_in,
                    cherokee_buffer_t *buffer)
{
	off_t to_read;

	if (post->encoding == post_enc_chunked) {
		return do_read_chunked (post, sock_in, buffer);
	}

	to_read = MIN((post->len - post->send.read), POST_READ_SIZE);
	return do_read_plain (post, sock_in, buffer, to_read);
}


int
cherokee_post_read_finished (cherokee_post_t *post)
{
	switch (post->encoding) {
	case post_enc_regular:
		return (post->send.read >= post->len);
	case post_enc_chunked:
		return post->chunked.last;
	default:
		break;
	}

	SHOULDNT_HAPPEN;
	return 1;
}


static ret_t
do_send_socket (cherokee_socket_t        *sock,
                cherokee_buffer_t        *buffer,
                cherokee_socket_status_t *blocking)
{
	ret_t  ret;
	size_t written = 0;

	ret = cherokee_socket_bufwrite (sock, buffer, &written);
	switch (ret) {
	case ret_ok:
		break;
	case ret_eagain:
		if (written > 0) {
			break;
		}

		TRACE (ENTRIES, "Post write: EAGAIN, wrote nothing of %d\n", buffer->len);
		*blocking = socket_writing;
		return ret_eagain;
	default:
		return ret_error;
	}

	cherokee_buffer_move_to_begin (buffer, written);
	TRACE (ENTRIES, "sent=%d, remaining=%d\n", written, buffer->len);

	if (! cherokee_buffer_is_empty (buffer)) {
		return ret_eagain;
	}

	return ret_ok;
}


int
cherokee_post_has_buffered_info (cherokee_post_t *post)
{
	return (! cherokee_buffer_is_empty (&post->send.buffer));
}


ret_t
cherokee_post_send_to_socket (cherokee_post_t          *post,
                              cherokee_socket_t        *sock_in,
                              cherokee_socket_t        *sock_out,
                              cherokee_buffer_t        *tmp,
                              cherokee_socket_status_t *blocking,
                              cherokee_boolean_t       *did_IO)
{
	ret_t              ret;
	cherokee_buffer_t *buffer = tmp ? tmp : &post->send.buffer;

	switch (post->send.phase) {
	case cherokee_post_send_phase_read:
		TRACE (ENTRIES, "Post send, phase: %s\n", "read");

		/* Read from the client
		 */
		ret = cherokee_post_read (post, sock_in, buffer);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			*blocking = socket_reading;
			return ret_eagain;
		default:
			return ret;
		}

		/* Did something, increase timeout
		 */
		*did_IO = true;

		/* Write it
		 */
		TRACE (ENTRIES, "Post buffer.len %d\n", buffer->len);
		post->send.phase = cherokee_post_send_phase_write;

	case cherokee_post_send_phase_write:
		TRACE (ENTRIES, "Post send, phase: write. Buffered: %d bytes\n", buffer->len);

		if (! cherokee_buffer_is_empty (buffer)) {
			ret = do_send_socket (sock_out, buffer, blocking);
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

			/* Did something, increase timeout
			 */
			*did_IO = true;
		}

		if (! cherokee_buffer_is_empty (buffer)) {
			return ret_eagain;
		}

		if (! cherokee_post_read_finished (post)) {
			post->send.phase = cherokee_post_send_phase_read;
			return ret_eagain;
		}

		TRACE (ENTRIES, "Post send: %s\n", "finished");

		cherokee_buffer_mrproper (&post->send.buffer);
		return ret_ok;

	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}


ret_t
cherokee_post_send_to_fd (cherokee_post_t          *post,
                          cherokee_socket_t        *sock_in,
                          int                       fd_out,
                          cherokee_buffer_t        *tmp,
                          cherokee_socket_status_t *blocking,
                          cherokee_boolean_t       *did_IO)
{
	ret_t              ret;
	int                r;
	cherokee_buffer_t *buffer = tmp ? tmp : &post->send.buffer;


	switch (post->send.phase) {
	case cherokee_post_send_phase_read:
		TRACE (ENTRIES, "Post send, phase: %s\n", "read");

		/* Read from the client
		 */
		ret = cherokee_post_read (post, sock_in, buffer);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			*blocking = socket_reading;
			return ret_eagain;
		default:
			return ret;
		}

		/* Did something, increase timeout
		 */
		*did_IO = true;

		/* Write it
		 */
		TRACE (ENTRIES, "Post buffer.len %d\n", buffer->len);
		post->send.phase = cherokee_post_send_phase_write;

	case cherokee_post_send_phase_write:
		TRACE (ENTRIES, "Post send, phase: write. Has %d bytes to send\n", buffer->len);

		if (! cherokee_buffer_is_empty (buffer)) {
			r = write (fd_out, buffer->buf, buffer->len);
			if (r < 0) {
				if (errno == EAGAIN) {
					*blocking = socket_writing;
					return ret_eagain;
				}

				TRACE(ENTRIES, "errno %d: %s\n", errno, strerror(errno));
				return ret_error;

			} else if (r == 0) {
				return ret_eagain;
			}

			cherokee_buffer_move_to_begin (buffer, r);

			/* Did something, increase timeout
			 */
			*did_IO = true;
		}

		/* Next iteration
		 */
		if (! cherokee_buffer_is_empty (buffer)) {
			return ret_eagain;
		}

		if (! cherokee_post_read_finished (post)) {
			post->send.phase = cherokee_post_send_phase_read;
			return ret_eagain;
		}

		TRACE (ENTRIES, "Post send: %s\n", "finished");

		cherokee_buffer_mrproper (&post->send.buffer);
		return ret_ok;

	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}
