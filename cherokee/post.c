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
#include "post.h"
#include "util.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define ENTRIES "post"


ret_t 
cherokee_post_init (cherokee_post_t *post)
{
	post->type        = post_undefined;
	post->size        = 0;
	post->received    = 0;
	post->tmp_file_p  = NULL;
	post->walk_offset = 0;
	
	cherokee_buffer_init (&post->info);
	cherokee_buffer_init (&post->tmp_file);
	
	return ret_ok;
}


ret_t 
cherokee_post_mrproper (cherokee_post_t *post)
{
	post->type        = post_undefined;
	post->size        = 0;
	post->received    = 0;
	post->walk_offset = 0;
	
	if (post->tmp_file_p != NULL) {
		fclose (post->tmp_file_p);
		post->tmp_file_p = NULL;
	}
	
	if (!cherokee_buffer_is_empty (&post->tmp_file)) {
		unlink (post->tmp_file.buf);
		cherokee_buffer_mrproper (&post->tmp_file);	   
	}

	cherokee_buffer_mrproper (&post->info);
       	
	return ret_ok;
}


ret_t 
cherokee_post_set_len (cherokee_post_t *post, size_t len)
{
	post->type = (len > POST_SIZE_TO_DISK) ? post_in_tmp_file : post_in_memory;
	post->size = len;

	if (post->type == post_in_tmp_file) {
		char *ptr;
		char  template[64];
		
		strncpy (template, "/tmp/cherokee_post_XXXXXX", 64); 
		
		/* Generate a unique name
		 */
		ptr = mktemp (template);
		if (unlikely (ptr == NULL)) 
			return ret_error;
		
		cherokee_buffer_add (&post->tmp_file, ptr, strlen(ptr));

		/* Open the file for writting
		 */
		post->tmp_file_p = fopen (ptr, "w+");
		if (unlikely (post->tmp_file_p == NULL)) 
			return ret_error;
	}
	
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
	return (post->received >= post->size);
}

ret_t 
cherokee_post_get_len (cherokee_post_t *post, size_t *len)
{
	*len = post->size;
	return ret_ok;
}


ret_t 
cherokee_post_append (cherokee_post_t *post, char *str, size_t len)
{
	cherokee_buffer_add (&post->info, str, len);
	cherokee_post_commit_buf (post, len);
	return ret_ok;
}


ret_t 
cherokee_post_commit_buf (cherokee_post_t *post, size_t size)
{
	size_t written;

	if (size <= 0)
		return ret_ok;

	switch (post->type) {
	case post_undefined:
		return ret_error;

	case post_in_memory:
		post->received += size;
		return ret_ok;
		
	case post_in_tmp_file:
		if (post->tmp_file_p == NULL)
			return ret_error;

		written = fwrite (post->info.buf, 1, post->info.len, post->tmp_file_p);
		if (written < 0) return ret_error;

		cherokee_buffer_move_to_begin (&post->info, written);
		post->received += written;

		return ret_ok;
	}

	return ret_error;
}


ret_t 
cherokee_post_walk_reset (cherokee_post_t *post)
{
	post->walk_offset = 0;

	if (post->tmp_file_p != NULL) {
		fseek (post->tmp_file_p, 0L, SEEK_SET);
	}

	return ret_ok;
}


ret_t 
cherokee_post_walk_finished (cherokee_post_t *post)
{
	switch (post->type) {
	case post_in_memory:
		return (post->walk_offset >= post->info.len) ? ret_ok : ret_eagain;
	case post_in_tmp_file:
		return (post->walk_offset >= post->size) ? ret_ok : ret_eagain;
	default:
		break;
	}
	
	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t 
cherokee_post_walk_to_fd (cherokee_post_t *post, int fd, int *eagain_fd, int *mode)
{
	ssize_t r;
	size_t  ur;
	
	/* Sanity check
	 */
	if (fd < 0) return ret_error;

	/* Send a chunk..
	 */
	switch (post->type) {
	case post_in_memory:
		r = write (fd, post->info.buf + post->walk_offset,
			   post->info.len - post->walk_offset);
		if (r < 0) {
			if (errno == EAGAIN)
				return ret_eagain;

			TRACE(ENTRIES, "errno %d\n", errno);
			return  ret_error; 			
		}

		TRACE(ENTRIES, "wrote %d\n", r);

		post->walk_offset += r;
		return cherokee_post_walk_finished (post);

	case post_in_tmp_file:
		cherokee_buffer_ensure_size (&post->info, DEFAULT_READ_SIZE);

		/* Read from the temp file is needed
		 */
		if (cherokee_buffer_is_empty (&post->info)) {
			ur = fread (post->info.buf, 1, DEFAULT_READ_SIZE - 1, post->tmp_file_p);
			if (ur <= 0) {
				return (feof(post->tmp_file_p)) ? ret_ok : ret_error;
			}
			
			post->info.len     = ur;
			post->info.buf[ur] = '\0';
		}

		/* Write it to the fd
		 */
		r = write (fd, post->info.buf, post->info.len);
		if (r < 0) {
			if (errno == EAGAIN) {
				*eagain_fd = fd;
				*mode      = 1;
				return ret_eagain;
			}

			return ret_error;	
		} 
		else if (r == 0)
			return ret_eagain;

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
cherokee_post_walk_read (cherokee_post_t *post, cherokee_buffer_t *buf, cuint_t len)
{
	size_t ur;

	switch (post->type) {
	case post_in_memory:
		if (len > (post->info.len - post->walk_offset))
			len = post->info.len - post->walk_offset;

		cherokee_buffer_add (buf, post->info.buf + post->walk_offset, len);
		post->walk_offset += len;
		break;

	case post_in_tmp_file:
		cherokee_buffer_ensure_size (buf, buf->len + len + 1);

		ur = fread (buf->buf + buf->len, 1, len, post->tmp_file_p);
		if (ur <= 0) {
			return (feof(post->tmp_file_p)) ? ret_ok : ret_error;
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

