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

#ifndef CHEROKEE_HANDLER_FILE_H
#define CHEROKEE_HANDLER_FILE_H

#include "common-internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "buffer.h"
#include "handler.h"
#include "connection.h"
#include "mime.h"


typedef struct {
	cherokee_handler_t handler;
	
	int                    fd;
	off_t                  offset;
	struct stat           *info;
	cherokee_mime_entry_t *mime;
	cherokee_boolean_t     using_sendfile;
	cherokee_boolean_t     use_cache;
	struct stat            cache_info;	
} cherokee_handler_file_t;

#define FHANDLER(x)  ((cherokee_handler_file_t *)(x))


/* Library init function
 */
void  MODULE_INIT(file)         (cherokee_module_loader_t *loader);
ret_t cherokee_handler_file_new (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_table_t *properties);

/* Virtual methods
 */
ret_t cherokee_handler_file_init        (cherokee_handler_file_t *hdl);
ret_t cherokee_handler_file_free        (cherokee_handler_file_t *hdl);
void  cherokee_handler_file_get_name    (cherokee_handler_file_t *hdl, const char **name);
ret_t cherokee_handler_file_step        (cherokee_handler_file_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_file_add_headers (cherokee_handler_file_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_FILE_H */
