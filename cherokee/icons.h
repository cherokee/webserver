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

#ifndef CHEROKEE_ICONS_H
#define CHEROKEE_ICONS_H

#include "common.h"
#include "avl.h"
#include "list.h"
#include "buffer.h"
#include "config_node.h"


typedef struct {
	cherokee_avl_t    files;
	cherokee_avl_t    suffixes;
	cherokee_list_t   file_matching;

	cherokee_buffer_t default_icon;
	cherokee_buffer_t directory_icon;
	cherokee_buffer_t parentdir_icon;
} cherokee_icons_t;

#define ICONS(x)  ((cherokee_icons_t *)(x))


ret_t cherokee_icons_new   (cherokee_icons_t **icons);
ret_t cherokee_icons_free  (cherokee_icons_t  *icons);
ret_t cherokee_icons_clean (cherokee_icons_t  *icons);

ret_t cherokee_icons_configure (cherokee_icons_t *icons, cherokee_config_node_t *config);
ret_t cherokee_icons_get_icon  (cherokee_icons_t *icons, cherokee_buffer_t *file, cherokee_buffer_t **icon);

ret_t cherokee_icons_add_file   (cherokee_icons_t *icons, cherokee_buffer_t *icon, cherokee_buffer_t *file);
ret_t cherokee_icons_add_suffix (cherokee_icons_t *icons, cherokee_buffer_t *icon, cherokee_buffer_t *suffix);

ret_t cherokee_icons_set_default   (cherokee_icons_t *icons, cherokee_buffer_t *icon);
ret_t cherokee_icons_set_directory (cherokee_icons_t *icons, cherokee_buffer_t *icon);
ret_t cherokee_icons_set_parentdir (cherokee_icons_t *icons, cherokee_buffer_t *icon);

#endif /* CHEROKEE_ICONS_H */
