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

#ifndef CHEROKEE_POST_H
#define CHEROKEE_POST_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <unistd.h>


CHEROKEE_BEGIN_DECLS

typedef enum {
	post_undefined,
	post_in_memory,
	post_in_tmp_file
} cherokee_post_type_t;

typedef struct {
	cherokee_post_type_t type;
	off_t                size;
	off_t                received;
	off_t                walk_offset;

	cherokee_buffer_t    info;

	cherokee_buffer_t    tmp_file;
	int                  tmp_file_fd;
} cherokee_post_t;

#define POST(x)      ((cherokee_post_t *)(x))
#define POST_BUF(x)  (&POST(x)->info)


ret_t cherokee_post_init          (cherokee_post_t *post);
ret_t cherokee_post_mrproper      (cherokee_post_t *post);

int   cherokee_post_is_empty      (cherokee_post_t *post);
int   cherokee_post_got_all       (cherokee_post_t *post);

ret_t cherokee_post_set_len       (cherokee_post_t *post, off_t  len);
ret_t cherokee_post_get_len       (cherokee_post_t *post, off_t *len);

ret_t cherokee_post_append        (cherokee_post_t *post, char *str, size_t len);
ret_t cherokee_post_commit_buf    (cherokee_post_t *post, size_t len);

ret_t cherokee_post_walk_reset    (cherokee_post_t *post);
ret_t cherokee_post_walk_finished (cherokee_post_t *post);
ret_t cherokee_post_walk_read     (cherokee_post_t *post, cherokee_buffer_t *buf, cuint_t len);
ret_t cherokee_post_walk_to_fd    (cherokee_post_t *post, int fd, int *eagain_fd, int *mode);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_POST_H */
