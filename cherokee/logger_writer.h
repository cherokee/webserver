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
	cherokee_logger_writer_types_t type;

	int                            fd;         /* file and pipe   */
	size_t                         max_bufsize;/* max. size of buffer */
	cherokee_buffer_t              buffer;

	cherokee_buffer_t              filename;  /* file            */
	cherokee_buffer_t              command;   /* pipe            */

} cherokee_logger_writer_t;


ret_t cherokee_logger_writer_init      (cherokee_logger_writer_t *writer);
ret_t cherokee_logger_writer_mrproper  (cherokee_logger_writer_t *writer);
ret_t cherokee_logger_writer_configure (cherokee_logger_writer_t *writer, cherokee_config_node_t *conf);

ret_t cherokee_logger_writer_open      (cherokee_logger_writer_t *writer);
ret_t cherokee_logger_writer_reopen    (cherokee_logger_writer_t *writer);

ret_t cherokee_logger_writer_get_buf   (cherokee_logger_writer_t *writer, cherokee_buffer_t **buf);
ret_t cherokee_logger_writer_flush     (cherokee_logger_writer_t *writer);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_LOGGER_WRITER_H */


