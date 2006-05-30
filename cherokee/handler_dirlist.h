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

#ifndef CHEROKEE_DIRLIST_HANDLER_H
#define CHEROKEE_DIRLIST_HANDLER_H

#include "common-internal.h"

#include <dirent.h>
#include <unistd.h>

#include "list.h"
#include "buffer.h"
#include "handler.h"
#include "module_loader.h"


typedef enum {
	Name_Down,
	Name_Up,
	Size_Down,
	Size_Up,
	Date_Down,
	Date_Up
} cherokee_dirlist_sort_t;

typedef enum {
	dirlist_phase_add_header,
	dirlist_phase_add_entries,
	dirlist_phase_add_footer
} cherokee_dirlist_phase_t;


typedef struct {
	cherokee_handler_props_t props;

	list_t                   notice_files;

	/* Visible properties
	 */
 	cherokee_boolean_t       show_size;
	cherokee_boolean_t       show_date;
	cherokee_boolean_t       show_user;
	cherokee_boolean_t       show_group;

	/* Theme
	 */
	cherokee_buffer_t        header;
	cherokee_buffer_t        footer;
	cherokee_buffer_t        entry;
	cherokee_buffer_t        css;
} cherokee_handler_dirlist_props_t;


typedef struct {
	cherokee_handler_t       handler;

	/* File list
	 */
	list_t                   dirs;
	list_t                   files;
	
	/* State
	 */
	cherokee_dirlist_sort_t  sort;
	cherokee_dirlist_phase_t phase;

	/* State
	 */
	cuint_t                  longest_filename;
	list_t                  *dir_ptr;
	list_t                  *file_ptr;	
 	cherokee_buffer_t        header;
	cherokee_boolean_t       serve_css;

	cherokee_buffer_t        public_dir;
	cherokee_buffer_t        server_software;
} cherokee_handler_dirlist_t;

#define PROP_DIRLIST(x)      ((cherokee_handler_dirlist_props_t *)(x))
#define HDL_DIRLIST(x)       ((cherokee_handler_dirlist_t *)(x))
#define HDL_DIRLIST_PROP(x)  (PROP_DIRLIST(HANDLER(x)->props))


/* Library init function
 */
void MODULE_INIT(dirlist)                  (cherokee_module_loader_t *loader);

ret_t cherokee_handler_dirlist_new         (cherokee_handler_t **hdl, void *cnt, cherokee_handler_props_t *properties);
ret_t cherokee_handler_dirlist_configure   (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_handler_props_t **_props);
ret_t cherokee_handler_dirlist_props_free  (cherokee_handler_dirlist_props_t *props);

/* virtual methods implementation
 */
ret_t cherokee_handler_dirlist_init        (cherokee_handler_dirlist_t *dhdl);
ret_t cherokee_handler_dirlist_free        (cherokee_handler_dirlist_t *dhdl);
void  cherokee_handler_dirlist_get_name    (cherokee_handler_dirlist_t *dhdl, char **name);
ret_t cherokee_handler_dirlist_step        (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_dirlist_add_headers (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_DIRLIST_HANDLER_H */
