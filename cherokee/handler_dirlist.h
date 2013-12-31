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

#ifndef CHEROKEE_DIRLIST_HANDLER_H
#define CHEROKEE_DIRLIST_HANDLER_H

#include "common-internal.h"

#include <dirent.h>
#include <unistd.h>

#include "list.h"
#include "buffer.h"
#include "handler.h"
#include "plugin_loader.h"


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
	dirlist_phase_add_parent_dir,
	dirlist_phase_add_entries,
	dirlist_phase_add_footer,
	dirlist_phase_finished
} cherokee_dirlist_phase_t;


typedef struct {
	cherokee_handler_props_t  props;

	cherokee_list_t          notice_files;
	cherokee_list_t          hidden_files;

	/* Visible properties
	 */
	cherokee_boolean_t       show_size;
	cherokee_boolean_t       show_date;
	cherokee_boolean_t       show_user;
	cherokee_boolean_t       show_group;
	cherokee_boolean_t       show_icons;
	cherokee_boolean_t       show_symlinks;
	cherokee_boolean_t       show_hidden;
	cherokee_boolean_t       show_backup;

	/* Theme
	 */
	cherokee_buffer_t        header;
	cherokee_buffer_t        footer;
	cherokee_buffer_t        entry;
	cherokee_buffer_t        css;

	/* Paths
	 */
	cherokee_boolean_t       redir_symlinks;
	cherokee_buffer_t        icon_web_dir;
} cherokee_handler_dirlist_props_t;


typedef struct {
	cherokee_handler_t       handler;

	/* File list
	 */
	cherokee_list_t          dirs;
	cherokee_list_t          files;

	/* State
	 */
	cherokee_dirlist_sort_t  sort;
	cherokee_dirlist_phase_t phase;

	/* State
	 */
	cuint_t                  longest_filename;
	cherokee_list_t         *dir_ptr;
	cherokee_list_t         *file_ptr;
	cherokee_buffer_t        header;

	cherokee_buffer_t        public_dir;
} cherokee_handler_dirlist_t;

#define PROP_DIRLIST(x)      ((cherokee_handler_dirlist_props_t *)(x))
#define HDL_DIRLIST(x)       ((cherokee_handler_dirlist_t *)(x))
#define HDL_DIRLIST_PROP(x)  (PROP_DIRLIST(MODULE(x)->props))


/* Library init function
 */
void PLUGIN_INIT_NAME(dirlist)             (cherokee_plugin_loader_t *loader);

ret_t cherokee_handler_dirlist_new         (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *properties);
ret_t cherokee_handler_dirlist_configure   (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props);
ret_t cherokee_handler_dirlist_props_free  (cherokee_handler_dirlist_props_t *props);

/* virtual methods implementation
 */
ret_t cherokee_handler_dirlist_init        (cherokee_handler_dirlist_t *dhdl);
ret_t cherokee_handler_dirlist_free        (cherokee_handler_dirlist_t *dhdl);
void  cherokee_handler_dirlist_get_name    (cherokee_handler_dirlist_t *dhdl, char **name);
ret_t cherokee_handler_dirlist_step        (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_dirlist_add_headers (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_DIRLIST_HANDLER_H */
