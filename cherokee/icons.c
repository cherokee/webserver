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

#include "common-internal.h"
#include "icons.h"
#include "list_ext.h"
#include "match.h"

extern int  yy_icons_parse             (void *);
extern int  yy_icons_restart           (FILE *);

extern int  yy_icons__create_buffer    (FILE *, int size);
extern void yy_icons__switch_to_buffer (void *);
extern void yy_icons__delete_buffer    (void *);
extern int  yy_icons__scan_string      (const char *);

ret_t 
cherokee_icons_new  (cherokee_icons_t **icons)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n, icons);

	ret = cherokee_table_init_case (&n->files);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_table_init (&n->files_matching);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_table_init_case (&n->suffixes);
	if (unlikely(ret < ret_ok)) return ret;

	n->default_icon   = NULL;
	n->directory_icon = NULL;
	n->parentdir_icon = NULL;
	
	*icons = n;
	return ret_ok;
}


ret_t
cherokee_icons_free (cherokee_icons_t *icons)
{
	/* TODO: Set a free_item function.
	 */
	cherokee_table_mrproper2 (&icons->files, free);
	cherokee_table_mrproper2 (&icons->suffixes, free);
	cherokee_table_mrproper2 (&icons->files_matching, free);

	if (icons->default_icon != NULL) {
		free (icons->default_icon);
		icons->default_icon = NULL;
	}

	if (icons->directory_icon != NULL) {
		free (icons->directory_icon);
		icons->directory_icon = NULL;
	}

	if (icons->parentdir_icon != NULL) {
		free (icons->parentdir_icon);
		icons->parentdir_icon = NULL;
	}

	free (icons);

	return ret_ok;
}


ret_t 
cherokee_icons_read_config_file (cherokee_icons_t *icons, char *filename)
{
	int   error;
	char *file;
	void *bufstate;

	extern FILE *yy_icons_in;

	file = (filename) ? filename : CHEROKEE_CONFDIR"/icons.conf";

	yy_icons_in = fopen (file, "r");
	if (yy_icons_in == NULL) {
		PRINT_ERROR("Can't read the icons file: '%s'\n", file);
		return ret_error;
	}

	yy_icons_restart (yy_icons_in);

	bufstate = (void *) yy_icons__create_buffer (yy_icons_in, 65535);
	yy_icons__switch_to_buffer (bufstate);
	error = yy_icons_parse (icons);
	yy_icons__delete_buffer (bufstate);

	fclose (yy_icons_in);

	return (error)? ret_error : ret_ok;
}


ret_t 
cherokee_icons_read_config_string (cherokee_icons_t *icons, const char *string)
{
	int   error;
	void *bufstate;

	extern int  yy_icons_parse (void *);

	bufstate = (void *) yy_icons__scan_string (string);
	yy_icons__switch_to_buffer(bufstate);

	error = yy_icons_parse((void *)icons);

	yy_icons__delete_buffer (bufstate);

	return (error)? ret_error : ret_ok;
}


ret_t 
cherokee_icons_set_suffixes (cherokee_icons_t *icons, list_t *suf_list, char *icon)
{
	list_t *i;

	/* Add suffixes to the table
	 */
	list_for_each (i, suf_list) {
		cherokee_table_add (&icons->suffixes, LIST_ITEM_INFO(i), strdup(icon));
	}
	
	return ret_ok;
}


ret_t 
cherokee_icons_set_files (cherokee_icons_t *icons, list_t *nam_list, char *icon)
{
	list_t *i;

	/* Add names to the table
	 */
	list_for_each (i, nam_list) {
		char *filename = LIST_ITEM_INFO(i);

		if ((strchr(filename, '*') != NULL) ||
		    (strchr(filename, '?') != NULL))
		{
			cherokee_table_add (&icons->files_matching, filename, strdup(icon));
			continue;
		}

		cherokee_table_add (&icons->files, filename, strdup(icon));
	}

	return ret_ok;
}


ret_t 
cherokee_icons_set_directory (cherokee_icons_t *icons, char *icon)
{
	if (icons->directory_icon != NULL) {
		free (icons->directory_icon);
		icons->directory_icon = NULL;
	}

	icons->directory_icon = icon;
	return ret_ok;
}

ret_t 
cherokee_icons_set_parentdir (cherokee_icons_t *icons, char *icon)
{
	if (icons->parentdir_icon != NULL) {
		free (icons->parentdir_icon);
		icons->parentdir_icon = NULL;
	}

	icons->parentdir_icon = icon;
	return ret_ok;
}

ret_t 
cherokee_icons_set_default (cherokee_icons_t *icons, char *icon)
{
	if (icons->default_icon != NULL) {
		free (icons->default_icon);
		icons->default_icon = NULL;
	}

	icons->default_icon = icon;
	return ret_ok;
}


static int
match_file (const char *pattern, 
	    void       *icon,
	    void       *param_file)
{
	return (! match (pattern, (char *)param_file));
}


ret_t 
cherokee_icons_get_icon (cherokee_icons_t *icons, char *file, char **icon_ret)
{
	ret_t   ret;
	char   *suffix, *match_string;

	/* Look for the filename
	 */
	ret = cherokee_table_get (&icons->files, file, (void **)icon_ret);
	if (ret == ret_ok) return ret_ok;

	/* Look for the suffix
	 */
	suffix = strrchr (file, '.');
	if (suffix != NULL) {
		ret = cherokee_table_get (&icons->suffixes, suffix+1, (void **)icon_ret);
		if (ret == ret_ok) return ret_ok;
	}
	
	/* Look for the wildcat matching
	 */
	ret = cherokee_table_while (&icons->files_matching, match_file, 
				    file, &match_string, (void **)icon_ret);
	if (ret == ret_ok) return ret_ok;

	/* Default one
	 */
	if (icons->default_icon != NULL) {
		*icon_ret = icons->default_icon;
	}

	return ret_ok;
}
