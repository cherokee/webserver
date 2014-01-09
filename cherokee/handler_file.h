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
#include "plugin_loader.h"

/* Data types
 */
typedef struct {
	cherokee_handler_props_t base;
	cherokee_boolean_t       use_cache;
	cherokee_boolean_t       send_symlinks;
} cherokee_handler_file_props_t;


typedef struct {
	cherokee_handler_t     handler;

	int                    fd;
	off_t                  offset;
	struct stat           *info;
	cherokee_mime_entry_t *mime;
	struct stat            cache_info;

	cherokee_boolean_t     using_sendfile;
	cherokee_boolean_t     not_modified;
} cherokee_handler_file_t;


#define PROP_FILE(x)      ((cherokee_handler_file_props_t *)(x))
#define HDL_FILE(x)       ((cherokee_handler_file_t *)(x))
#define HDL_FILE_PROP(x)  (PROP_FILE(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(file)            (cherokee_plugin_loader_t *loader);

ret_t cherokee_handler_file_new         (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_file_configure   (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props);
ret_t cherokee_handler_file_props_free  (cherokee_handler_file_props_t *props);

/* Virtual methods
 */
ret_t cherokee_handler_file_init        (cherokee_handler_file_t *hdl);
ret_t cherokee_handler_file_free        (cherokee_handler_file_t *hdl);
void  cherokee_handler_file_get_name    (cherokee_handler_file_t *hdl, const char **name);
ret_t cherokee_handler_file_step        (cherokee_handler_file_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_file_add_headers (cherokee_handler_file_t *hdl, cherokee_buffer_t *buffer);

/* Extre methods
 */
ret_t cherokee_handler_file_custom_init (cherokee_handler_file_t *hdl, cherokee_buffer_t *local_file);
ret_t cherokee_handler_file_seek        (cherokee_handler_file_t *hdl, off_t start);

#endif /* CHEROKEE_HANDLER_FILE_H */
