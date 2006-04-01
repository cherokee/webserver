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

#ifndef CHEROKEE_ICONS_H
#define CHEROKEE_ICONS_H

#include "common.h"
#include "table.h"
#include "table-protected.h"
#include "list.h"
#include "buffer.h"

typedef struct {
	cherokee_table_t files;
	cherokee_table_t suffixes;
	cherokee_table_t files_matching;	

	char *default_icon;
	char *directory_icon;
	char *parentdir_icon;
} cherokee_icons_t;

#define ICONS(x)  ((cherokee_icons_t *)(x))


ret_t cherokee_icons_new   (cherokee_icons_t **icons);
ret_t cherokee_icons_free  (cherokee_icons_t  *icons);
ret_t cherokee_icons_clean (cherokee_icons_t  *icons);

ret_t cherokee_icons_read_config_file   (cherokee_icons_t *icons, char *file);
ret_t cherokee_icons_read_config_string (cherokee_icons_t *icons, const char *string);
 
ret_t cherokee_icons_get_icon (cherokee_icons_t *icons, char *file, char **icon);

ret_t cherokee_icons_set_suffixes  (cherokee_icons_t *icons, list_t *suf_list, char *icon);
ret_t cherokee_icons_set_files     (cherokee_icons_t *icons, list_t *nam_list, char *icon);
ret_t cherokee_icons_set_default   (cherokee_icons_t *icons, char *icon);
ret_t cherokee_icons_set_directory (cherokee_icons_t *icons, char *icon);
ret_t cherokee_icons_set_parentdir (cherokee_icons_t *icons, char *icon);

#endif /* CHEROKEE_ICONS_H */
