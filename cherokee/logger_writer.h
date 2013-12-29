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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_LOGGER_WRITER_H
#define CHEROKEE_LOGGER_WRITER_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/config_node.h>

CHEROKEE_BEGIN_DECLS

#define LOGGER_BUF_PAGESIZE	4096	/* page size to round down write(n) */

typedef enum {
	cherokee_logger_writer_stderr,
	cherokee_logger_writer_file,
	cherokee_logger_writer_syslog,
	cherokee_logger_writer_pipe
} cherokee_logger_writer_types_t;

typedef struct {
	cherokee_list_t                listed;
	cherokee_logger_writer_types_t type;

	int                            fd;
	size_t                         max_bufsize;
	cherokee_buffer_t              buffer;

	cherokee_buffer_t              filename;
	cherokee_buffer_t              command;

	cherokee_boolean_t             initialized;

	void                          *priv;
} cherokee_logger_writer_t;

#define LOGGER_WRITER(x) ((cherokee_logger_writer_t *)(x))

ret_t cherokee_logger_writer_new         (cherokee_logger_writer_t **writer);
ret_t cherokee_logger_writer_new_stderr  (cherokee_logger_writer_t **writer);
ret_t cherokee_logger_writer_free        (cherokee_logger_writer_t  *writer);

ret_t cherokee_logger_writer_configure   (cherokee_logger_writer_t *writer, cherokee_config_node_t *conf);

ret_t cherokee_logger_writer_open        (cherokee_logger_writer_t *writer);
ret_t cherokee_logger_writer_reopen      (cherokee_logger_writer_t *writer);
ret_t cherokee_logger_writer_flush       (cherokee_logger_writer_t *writer, cherokee_boolean_t locked);

ret_t cherokee_logger_writer_get_buf     (cherokee_logger_writer_t *writer, cherokee_buffer_t **buf);
ret_t cherokee_logger_writer_release_buf (cherokee_logger_writer_t *writer);

/* Extra */
ret_t cherokee_logger_writer_get_id (cherokee_config_node_t *conf, cherokee_buffer_t *id);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_LOGGER_WRITER_H */


