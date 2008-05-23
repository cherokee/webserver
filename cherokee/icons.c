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

#include "common-internal.h"

#include "icons.h"
#include "match.h"
#include "util.h"

#define ENTRIES "icons"


typedef struct {
	cherokee_list_t   entry;
	cherokee_buffer_t match;
	cherokee_buffer_t icon;
} file_matching_t;

static file_matching_t *
matching_new (cherokee_buffer_t *file,
	      cherokee_buffer_t *icon)
{
	file_matching_t *n;

	n = (file_matching_t *)malloc (sizeof(file_matching_t));
	INIT_LIST_HEAD (&n->entry);

	cherokee_buffer_init (&n->match);
	cherokee_buffer_init (&n->icon);
	cherokee_buffer_add_buffer (&n->match, file);
	cherokee_buffer_add_buffer (&n->icon, icon);

	return n;
}

static void
matching_free (file_matching_t *match)
{
	cherokee_buffer_mrproper (&match->match);
	cherokee_buffer_mrproper (&match->icon);
	free (match);
}



ret_t 
cherokee_icons_new (cherokee_icons_t **icons)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n, icons);

	INIT_LIST_HEAD (&n->file_matching);

	ret = cherokee_avl_init (&n->files);
	if (unlikely(ret < ret_ok))
		return ret;

	ret = cherokee_avl_init (&n->suffixes);
	if (unlikely(ret < ret_ok))
		return ret;

	/* Files and Suffixes are case insensitive
	 */
	cherokee_avl_set_case (&n->files, true);
	cherokee_avl_set_case (&n->suffixes, true);

	/* Defaults
	 */
	cherokee_buffer_init (&n->default_icon);
	cherokee_buffer_init (&n->directory_icon);
	cherokee_buffer_init (&n->parentdir_icon);

	*icons = n;
	return ret_ok;
}


static void
free_entry (void *param)
{
	cherokee_buffer_free (BUF(param));
}


ret_t
cherokee_icons_free (cherokee_icons_t *icons)
{
	cherokee_list_t *i, *j;

	if (unlikely (icons == NULL))
		return ret_ok;

	/* Free the list and AVL trees
	 */
	list_for_each_safe (i, j, &icons->file_matching) {
		matching_free((file_matching_t *)i);
	}

	cherokee_avl_mrproper (&icons->files, free_entry);
	cherokee_avl_mrproper (&icons->suffixes, free_entry);

	/* Free the special icons buffers
	 */
	cherokee_buffer_mrproper (&icons->default_icon);
	cherokee_buffer_mrproper (&icons->directory_icon);
	cherokee_buffer_mrproper (&icons->parentdir_icon);

	free (icons);
	return ret_ok;
}


ret_t 
cherokee_icons_add_file (cherokee_icons_t *icons, cherokee_buffer_t *icon, cherokee_buffer_t *file)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	/* Add a wildcards entry
	 */
	if ((strchr (file->buf, '*') != NULL) ||
	    (strchr (file->buf, '?') != NULL)) 
	{
		file_matching_t *match;

		match = matching_new (file, icon);
		if (unlikely (match == NULL))
			return ret_nomem;

		cherokee_list_add (&match->entry, &icons->file_matching);
		return ret_ok;
	} 

	/* Add a plain name
	 */
	ret = cherokee_buffer_dup (icon, &tmp);
	if (unlikely (ret != ret_ok))
		return ret;

	return cherokee_avl_add (&icons->files, file, tmp);
}


ret_t 
cherokee_icons_add_suffix (cherokee_icons_t *icons, cherokee_buffer_t *icon, cherokee_buffer_t *suffix)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	ret = cherokee_buffer_dup (icon, &tmp);
	if (unlikely (ret != ret_ok))
		return ret;

	cherokee_avl_add (&icons->suffixes, suffix, tmp);
	return ret_ok;
}


ret_t 
cherokee_icons_set_directory (cherokee_icons_t *icons, cherokee_buffer_t *icon)
{
	cherokee_buffer_clean (&icons->directory_icon);
	return cherokee_buffer_add_buffer (&icons->directory_icon, icon);
}


ret_t 
cherokee_icons_set_parentdir (cherokee_icons_t *icons, cherokee_buffer_t *icon)
{
	cherokee_buffer_clean (&icons->parentdir_icon);
	return cherokee_buffer_add_buffer (&icons->parentdir_icon, icon);
}


ret_t 
cherokee_icons_set_default (cherokee_icons_t *icons, cherokee_buffer_t *icon)
{
	cherokee_buffer_clean (&icons->default_icon);
	return cherokee_buffer_add_buffer (&icons->default_icon, icon);
}


ret_t 
cherokee_icons_get_icon (cherokee_icons_t   *icons, 
			 cherokee_buffer_t  *file,
			 cherokee_buffer_t **icon_ret)
{
	ret_t            ret;
	char            *suffix;
	cherokee_list_t *i;

	/* Look for the filename
	 */
	ret = cherokee_avl_get (&icons->files, file, (void **)icon_ret);
	if (ret == ret_ok)
		return ret_ok;

	/* Look for the suffix
	 */
	suffix = strrchr (file->buf, '.');
	if (suffix != NULL) {
		ret = cherokee_avl_get_ptr (&icons->suffixes, suffix+1, (void **)icon_ret);
		if (ret == ret_ok)
			return ret_ok;
	}
	
	/* Look for the wildcard matching
	 */
	list_for_each (i, &icons->file_matching) {
		file_matching_t *match = (file_matching_t *)i;
		
		ret = cherokee_wildcard_match (match->match.buf, file->buf);
		if (ret == ret_ok) {
			*icon_ret = &match->icon;
			return ret_ok;
		}
	}

	/* Use the default icon
	 */
	if (! cherokee_buffer_is_empty (&icons->default_icon)) {
		*icon_ret = &icons->default_icon;
		return ret_ok;
	}

	PRINT_MSG_S ("A default icons is needed\n");
	return ret_error;
}


static ret_t 
add_file (char *file, void *data)
{
	cherokee_buffer_t  file_buf;
	cherokee_icons_t  *icons    = ((void **)data)[0];
	cherokee_buffer_t *key      = ((void **)data)[1];

	TRACE(ENTRIES, "Adding file icon '%s' -> '%s'\n", key->buf, file);

	cherokee_buffer_fake (&file_buf, file, strlen(file));
	return cherokee_icons_add_file (icons, key, &file_buf);
}

static ret_t 
add_suffix (char *file, void *data)
{
	cherokee_buffer_t  file_buf;
	cherokee_icons_t  *icons = ((void **)data)[0];
	cherokee_buffer_t *key   = ((void **)data)[1];

	TRACE(ENTRIES, "Adding suffix icon '%s' -> '%s'\n", key->buf, file);
	
	cherokee_buffer_fake (&file_buf, file, strlen(file));
	return cherokee_icons_add_suffix (icons, key, &file_buf);
}


static ret_t 
configure_file (cherokee_config_node_t *config, void *data)
{
	ret_t  ret;
	void  *params[2];

	params[0] = data;
	params[1] = &config->key;

	ret = cherokee_config_node_read_list (config, NULL, add_file, params);
	if ((ret != ret_ok) && (ret != ret_not_found))
		return ret;
	
	return ret_ok;
}

static ret_t 
configure_suffix (cherokee_config_node_t *config, void *data)
{
	ret_t  ret;
	void  *params[2];

	params[0] = data;
	params[1] = &config->key;

	ret = cherokee_config_node_read_list (config, NULL, add_suffix, params);
	if ((ret != ret_ok) && (ret != ret_not_found))
		return ret;
	
	return ret_ok;
}


ret_t 
cherokee_icons_configure (cherokee_icons_t *icons, cherokee_config_node_t *config)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf;

	ret = cherokee_config_node_get (config, "file", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_config_node_while (subconf, configure_file, icons);
		if (ret != ret_ok)
			return ret;
	}

	ret = cherokee_config_node_get (config, "suffix", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_config_node_while (subconf, configure_suffix, icons);
		if (ret != ret_ok)
			return ret;
	}
	
	ret = cherokee_config_node_get (config, "directory", &subconf);
	if (ret == ret_ok) {
		cherokee_icons_set_directory (icons, &subconf->val);
	}
	
	ret = cherokee_config_node_get (config, "parent_directory", &subconf);
	if (ret == ret_ok) {
		cherokee_icons_set_parentdir (icons, &subconf->val);
	}
	
	ret = cherokee_config_node_get (config, "default", &subconf);
	if (ret == ret_ok) {
		cherokee_icons_set_default (icons, &subconf->val);
	}

	return ret_ok;
}
