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

#ifndef CHEROKEE_POST_H
#define CHEROKEE_POST_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/socket.h>
#include <unistd.h>


CHEROKEE_BEGIN_DECLS

typedef enum {
	post_undefined,
	post_in_memory,
	post_in_tmp_file
} cherokee_post_type_t;

typedef enum {
	post_enc_regular,
	post_enc_chunked
} cherokee_post_encoding_t;

typedef struct {
	cherokee_post_type_t     type;
	cherokee_post_encoding_t encoding;
	cherokee_boolean_t       is_set;

	cherokee_boolean_t       chunked_last;
	off_t                    chunked_processed;

	cherokee_boolean_t       got_last_chunk;

	off_t                    size;
	off_t                    received;
	off_t                    walk_offset;

	cherokee_buffer_t        info;

	cherokee_buffer_t        tmp_file;
	int                      tmp_file_fd;
} cherokee_post_t;

#define POST(x)      ((cherokee_post_t *)(x))
#define POST_BUF(x)  (&POST(x)->info)


ret_t cherokee_post_init           (cherokee_post_t *post);
ret_t cherokee_post_mrproper       (cherokee_post_t *post);

int   cherokee_post_is_empty       (cherokee_post_t *post);
int   cherokee_post_got_all        (cherokee_post_t *post);

ret_t cherokee_post_set_len        (cherokee_post_t *post, off_t  len);
ret_t cherokee_post_get_len        (cherokee_post_t *post, off_t *len);

ret_t cherokee_post_append         (cherokee_post_t *post, const char *str, size_t len, size_t *written);
ret_t cherokee_post_commit_buf     (cherokee_post_t *post, size_t len);
ret_t cherokee_post_set_encoding   (cherokee_post_t *post, cherokee_post_encoding_t enc);

ret_t cherokee_post_walk_reset     (cherokee_post_t *post);
ret_t cherokee_post_walk_finished  (cherokee_post_t *post);
ret_t cherokee_post_walk_read      (cherokee_post_t *post, cherokee_buffer_t *buf, cuint_t len);
ret_t cherokee_post_walk_to_fd     (cherokee_post_t *post, int fd, int *eagain_fd, int *mode);
ret_t cherokee_post_walk_to_socket (cherokee_post_t *post, cherokee_socket_t *socket);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_POST_H */
