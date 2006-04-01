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


enum cherokee_sort {
	Name_Down,
	Name_Up,
	Size_Down,
	Size_Up,
	Date_Down,
	Date_Up
};
typedef enum cherokee_sort cherokee_sort_t;


typedef struct {
	cherokee_handler_t handler;

	/* File list
	 */
	struct list_head dirs;
	struct list_head files;
	cherokee_sort_t  sort;

	/* State
	 */
	cherokee_boolean_t  page_header;
	cuint_t             longest_filename;
	list_t             *dir_ptr;
	list_t             *file_ptr;

	/* Properties
	 */
	char *bgcolor;     /* background color for the document */
	char *text;        /* color for the text of the document */
	char *link;        /* color for unvisited hypertext links */
	char *vlink;       /* color for visited hypertext links */
	char *alink;       /* color for active hypertext links */
	char *background;  /* URL for an image to be used to tile the background */

	cuint_t show_size;
	cuint_t show_date;
	cuint_t show_owner;
	cuint_t show_group;
	
	cherokee_buffer_t   header;          /* Header content */
	list_t             *header_file;     /* List of possible header filenames */
	char               *header_file_ref; /* Pointer to the used header filename */
	cherokee_boolean_t  build_headers;   /* Build headers */

} cherokee_handler_dirlist_t;

#define DHANDLER(x)  ((cherokee_handler_dirlist_t *)(x))


/* Library init function
 */
void MODULE_INIT(dirlist)          (cherokee_module_loader_t *loader);
ret_t cherokee_handler_dirlist_new (cherokee_handler_t **hdl, void *cnt, cherokee_table_t *properties);

/* virtual methods implementation
 */
ret_t cherokee_handler_dirlist_init        (cherokee_handler_dirlist_t *dhdl);
ret_t cherokee_handler_dirlist_free        (cherokee_handler_dirlist_t *dhdl);
void  cherokee_handler_dirlist_get_name    (cherokee_handler_dirlist_t *dhdl, char **name);
ret_t cherokee_handler_dirlist_step        (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_dirlist_add_headers (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer);


#endif /* CHEROKEE_DIRLIST_HANDLER_H */
