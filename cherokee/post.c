/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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
#include "util.h"
#include "init.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define ENTRIES "post"


ret_t 
cherokee_post_init (cherokee_post_t *post)
{
	post->type              = post_undefined;
	post->encoding          = post_enc_regular;
	post->is_set            = false;

	post->size              = 0;
	post->received          = 0;
	post->tmp_file_fd       = -1;
	post->walk_offset       = 0;

	post->chunked_last      = false;
	post->chunked_processed = 0;
	
	cherokee_buffer_init (&post->info);
	cherokee_buffer_init (&post->tmp_file);
	
	return ret_ok;
}


ret_t 
cherokee_post_mrproper (cherokee_post_t *post)
{
	int re;

	post->type              = post_undefined;
	post->encoding          = post_enc_regular;
	post->is_set            = false;

	post->size              = 0;
	post->received          = 0;
	post->walk_offset       = 0;

	post->chunked_last      = false;
	post->chunked_processed = 0;

	
	if (post->tmp_file_fd != -1) {
		close (post->tmp_file_fd);
		post->tmp_file_fd = -1;
	}
	
	if (! cherokee_buffer_is_empty (&post->tmp_file)) {
		re = unlink (post->tmp_file.buf);
		if (unlikely (re != 0)) {
			LOG_ERRNO (errno, cherokee_err_error, 
				   CHEROKEE_ERROR_POST_REMOVE_TEMP, post->tmp_file.buf);
		}

		TRACE(ENTRIES, "Removing '%s'\n", post->tmp_file.buf);
		cherokee_buffer_mrproper (&post->tmp_file);
	}

	cherokee_buffer_mrproper (&post->info);

	return ret_ok;
}


ret_t 
cherokee_post_set_len (cherokee_post_t *post, off_t len)
{
	ret_t ret;

	post->type   = (len > POST_SIZE_TO_DISK) ? post_in_tmp_file : post_in_memory;
	post->size   = len;
	post->is_set = true;

	if (post->type == post_in_tmp_file) {
		cherokee_buffer_add_buffer (&post->tmp_file, &cherokee_tmp_dir);
		cherokee_buffer_add_str    (&post->tmp_file, "/cherokee_post_XXXXXX");

		/* Generate a unique name
		 */
		ret = cherokee_mkstemp (&post->tmp_file, &post->tmp_file_fd);
		if (unlikely (ret != ret_ok))
			return ret;

		TRACE(ENTRIES, "Setting len=%d, to file='%s'\n", len, post->tmp_file.buf);
	} else {
		TRACE(ENTRIES, "Setting len=%d, to memory\n", len);
	}

	return ret_ok;
}


ret_t
cherokee_post_set_encoding (cherokee_post_t          *post,
			    cherokee_post_encoding_t  enc)
{
	TRACE(ENTRIES, "Setting encoding: %s\n",
	      (enc == post_enc_regular) ? "regular" : "chunked");

	post->type     = post_in_memory;
	post->encoding = enc;
	post->is_set   = true;

	return ret_ok;
}


int
cherokee_post_is_empty (cherokee_post_t *post)
{
	return (post->size <= 0);
}


int
cherokee_post_got_all (cherokee_post_t *post)
{
	switch(post->encoding) {
	case post_enc_regular:
		return (post->received >= post->size);
	case post_enc_chunked:
		return post->chunked_last;
	}

	SHOULDNT_HAPPEN;
	return 0;
}


ret_t 
cherokee_post_get_len (cherokee_post_t *post, off_t *len)
{
	*len = post->size;
	return ret_ok;
}


ret_t 
cherokee_post_append (cherokee_post_t *post, const char *str, size_t len, size_t *written)
{
	if ((post->size > 0) &&
	    (post->received + len > post->size))
	{
		*written = post->size - post->received;
	} else {
		*written = len;
	}

	TRACE(ENTRIES, "appends=%d bytes\n", *written);

	cherokee_buffer_add (&post->info, str, *written);
	cherokee_post_commit_buf (post, *written);
	return ret_ok;
}


static ret_t
process_chunk (cherokee_post_t *post, 
	       off_t            start_at,
	       off_t           *processed)
{
	char  *begin;
	char  *p;
	char  *end;
	off_t  content_size;

	begin = post->info.buf + start_at;
	p     = begin;

	TRACE (ENTRIES, "Post info buffer len=%d, starting at="FMT_OFFSET"\n",
	       post->info.len, start_at);

	while (true) {
		end = post->info.buf + post->info.len;

		/* Iterate through the number */
		while ((p < end) &&
		       (((*p >= '0') && (*p <= '9')) ||
			((*p >= 'a') && (*p <= 'f')) ||
			((*p >= 'A') && (*p <= 'F'))))
			p++;

		if (unlikely (p+2 > end))
			return ret_ok;

		/* Check the CRLF after the length */
		if (p[0] != CHR_CR)
			return ret_error;
		if (p[1] != CHR_LF)
			return ret_error;

		/* Read the length */
		content_size = (off_t) strtoul (begin, &p, 16);
		p += 2;
		
		if (unlikely (content_size < 0))
			return ret_error;
		
		/* Check if there's enough info */
		if (content_size == 0)
			post->chunked_last = true;

		else if (p + content_size + 2 > end) {
			TRACE (ENTRIES, "Unfinished chunk(len="FMT_OFFSET"), has=%d, processed="FMT_OFFSET"\n",
			       content_size, (int)(end-p), *processed);
			return ret_ok;
		}

		TRACE (ENTRIES, "Processing chunk len=%d\n", content_size);

		/* Remove the prefix */
		cherokee_buffer_remove_chunk (&post->info, begin - post->info.buf, p-begin);

		if (post->chunked_last) {
			TRACE(ENTRIES, "Last chunk: %s\n", "exiting");
			break;
		} else {
			/* Clean the trailing CRLF */
			p = post->info.buf + start_at + content_size;
			cherokee_buffer_remove_chunk (&post->info, p - post->info.buf, 2);
		}

		/* Next iteration */
		begin = p;
		*processed = (*processed + content_size);
	}

	return ret_ok;
}


ret_t 
cherokee_post_commit_buf (cherokee_post_t *post, size_t size)
{
	ret_t   ret;
	ssize_t written;
	off_t   processed = 0;

	if (unlikely (size <= 0))
		return ret_ok;

	/* Chunked post
	 */
	if (post->encoding == post_enc_chunked) {
		/* Process the chunks */
		ret = process_chunk (post, post->chunked_processed, &processed);
		if (unlikely (ret != ret_ok)) {
			return ret_error;
		}

		if (processed <= 0) {
			return ret_ok;
		}
		
		/* Store the processed info */
		switch (post->type) {
		case post_undefined:
			SHOULDNT_HAPPEN;
			return ret_error;

		case post_in_memory:
			post->received          += processed;
			post->chunked_processed += processed;
			break;

		case post_in_tmp_file:
			if (post->tmp_file_fd == -1)
				return ret_error;
			
			written = write (post->tmp_file_fd, post->info.buf, processed);
			if (unlikely ((written < 0) || (written < processed))) {
				return ret_error;
			}
						 
			cherokee_buffer_move_to_begin (&post->info, (cuint_t)written);

			post->chunked_processed -= written;
			post->received          += written;

			break;
		}

		if (post->chunked_last) {
			TRACE(ENTRIES, "Got it all: len=%d\n", post->received);
			post->size = post->received;
		}

		return ret_ok;
	}

	/* Plain Post
	 */
	switch (post->type) {
	case post_undefined:
		return ret_error;

	case post_in_memory:
		post->received += size;
		return ret_ok;

	case post_in_tmp_file:
		if (unlikely (post->tmp_file_fd == -1))
			return ret_error;

		written = write (post->tmp_file_fd, post->info.buf, post->info.len); 
		if (unlikely ((written < 0) || (written < processed))) {
			return ret_error;
		}

		cherokee_buffer_move_to_begin (&post->info, (cuint_t)written);
		post->received += written;

		return ret_ok;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t 
cherokee_post_walk_reset (cherokee_post_t *post)
{
	post->walk_offset = 0;

	if (post->tmp_file_fd != -1) {
		lseek (post->tmp_file_fd, 0L, SEEK_SET);
	}

	return ret_ok;
}


ret_t 
cherokee_post_walk_finished (cherokee_post_t *post)
{
	ret_t ret;

	switch (post->type) {
	case post_in_memory:
		ret = (post->walk_offset >= post->info.len) ? ret_ok : ret_eagain;
		break;
	case post_in_tmp_file:
		ret = (post->walk_offset >= post->size) ? ret_ok : ret_eagain;
		break;
	default:
		ret = ret_error;
		break;
	}

	TRACE(ENTRIES, "ret=%d\n", ret);
	return ret;
}

static ret_t
post_read_file_step (cherokee_post_t *post)
{
	int r;

	cherokee_buffer_ensure_size (&post->info, DEFAULT_READ_SIZE+1);
	
	/* Read from the temp file is needed
	 */
	if (!cherokee_buffer_is_empty (&post->info))
		return ret_ok;

	r = read (post->tmp_file_fd, post->info.buf, DEFAULT_READ_SIZE);
	if (r == 0) {
		/* EOF */
		return ret_ok;
	} else if (r < 0) {
		/* Couldn't read */
		return ret_error;
	}
	
	post->info.len    = r;
	post->info.buf[r] = '\0';
	return ret_ok;
}

ret_t 
cherokee_post_walk_to_fd (cherokee_post_t *post, int fd, int *eagain_fd, int *mode)
{
	ret_t   ret;
	ssize_t r;

	/* Sanity check
	 */
	if (fd < 0) {
		return ret_error;
	}

	/* Send a chunk..
	 */
	switch (post->type) {
	case post_in_memory:
		r = write (fd,
			   post->info.buf + post->walk_offset,
			   post->info.len - post->walk_offset);
		if (r < 0) {
			int err = errno;

			if (err == EAGAIN)
				return ret_eagain;

			TRACE(ENTRIES, "errno %d: %s\n", err, strerror(err));
			return  ret_error; 			
		} 

		TRACE(ENTRIES, "wrote %d bytes from memory\n", r);

		post->walk_offset += r;
		return cherokee_post_walk_finished (post);

	case post_in_tmp_file:
		ret = post_read_file_step (post);
		if (ret != ret_ok)
			return ret;

		/* Write it to the fd
		 */
		r = write (fd, post->info.buf, post->info.len);
		if (r < 0) {
			if (errno == EAGAIN) {
				if (eagain_fd) {
					*eagain_fd = fd;
				}
				if (mode) {
					*mode = 1;
				}
				return ret_eagain;
			}

			TRACE(ENTRIES, "errno %d\n", errno);
			return ret_error;	
		} 
		else if (r == 0)
			return ret_eagain;

		TRACE(ENTRIES, "wrote %d bytes from memory\n", r);
		cherokee_buffer_move_to_begin (&post->info, r);

		post->walk_offset += r;
		return cherokee_post_walk_finished (post);

	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_post_walk_to_socket (cherokee_post_t *post, cherokee_socket_t *socket)
{
	ret_t  ret;
	size_t written = 0;

	switch (post->type) {
	case post_in_memory:
		ret = cherokee_socket_write (socket, 
					     post->info.buf + post->walk_offset,
					     post->info.len - post->walk_offset,
					     &written);

		if (written > 0){
			TRACE(ENTRIES, "wrote %d bytes from memory\n", written);
			post->walk_offset += written;
			return cherokee_post_walk_finished (post);
		}
		return ret;

	case post_in_tmp_file:
		ret = post_read_file_step (post);
		if (ret != ret_ok)
			return ret;

		ret = cherokee_socket_write (socket, post->info.buf, post->info.len, &written);
		if (written > 0) {
			cherokee_buffer_move_to_begin (&post->info, written);
			post->walk_offset += written;
			return cherokee_post_walk_finished (post);
		}
		return ret;

	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}
	
	return ret_ok;
}


ret_t 
cherokee_post_walk_read (cherokee_post_t *post, cherokee_buffer_t *buf, cuint_t len)
{
	ssize_t ur;

	switch (post->type) {
	case post_in_memory:
		if (len > (post->info.len - post->walk_offset))
			len = post->info.len - post->walk_offset;

		cherokee_buffer_add (buf, post->info.buf + post->walk_offset, len);
		post->walk_offset += len;
		break;

	case post_in_tmp_file:
		cherokee_buffer_ensure_size (buf, buf->len + len + 1);

		ur = read (post->tmp_file_fd, buf->buf + buf->len, len);
		if (ur == 0) {
			/* EOF */
			return ret_ok;
		} else if (unlikely (ur < 0)) {
			/* Couldn't read */
			return ret_error;
		}

		buf->len           += ur;
		buf->buf[buf->len]  = '\0';
		post->walk_offset  += ur;
		break;

	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return cherokee_post_walk_finished (post);
}

