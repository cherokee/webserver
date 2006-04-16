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
#include "util.h"

#define ENTRIES "icons"


ret_t 
cherokee_icons_new (cherokee_icons_t **icons)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n, icons);

	ret = cherokee_table_init_case (&n->files);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_table_init (&n->files_matching);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_table_init_case (&n->suffixes);
	if (unlikely(ret < ret_ok)) return ret;

	cherokee_buffer_init (&n->default_icon);
	cherokee_buffer_init (&n->directory_icon);
	cherokee_buffer_init (&n->parentdir_icon);
	
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

	cherokee_buffer_mrproper (&icons->default_icon);
	cherokee_buffer_mrproper (&icons->directory_icon);
	cherokee_buffer_mrproper (&icons->parentdir_icon);

	free (icons);
	return ret_ok;
}


ret_t 
cherokee_icons_add_file (cherokee_icons_t *icons, cherokee_buffer_t *icon, const char *file)
{
	if ((strchr(file, '*') != NULL) ||
	    (strchr(file, '?') != NULL)) 
	{
		cherokee_table_add (&icons->files_matching, (char *)file, strdup(icon->buf));
		return ret_ok;
	}
	
	cherokee_table_add (&icons->files, (char *)file, strdup(icon->buf));
	return ret_ok;
}


ret_t 
cherokee_icons_add_suffix (cherokee_icons_t *icons, cherokee_buffer_t *icon, const char *suffix)
{
	cherokee_table_add (&icons->suffixes, (char *)suffix, strdup(icon->buf));
	return ret_ok;
}


ret_t 
cherokee_icons_set_directory (cherokee_icons_t *icons, cherokee_buffer_t *icon)
{
	cherokee_buffer_clean (&icons->directory_icon);
	cherokee_buffer_add_buffer (&icons->directory_icon, icon);
	return ret_ok;
}


ret_t 
cherokee_icons_set_parentdir (cherokee_icons_t *icons, cherokee_buffer_t *icon)
{
	cherokee_buffer_clean (&icons->parentdir_icon);
	cherokee_buffer_add_buffer (&icons->parentdir_icon, icon);
	return ret_ok;
}


ret_t 
cherokee_icons_set_default (cherokee_icons_t *icons, cherokee_buffer_t *icon)
{
	cherokee_buffer_clean (&icons->default_icon);
	cherokee_buffer_add_buffer (&icons->default_icon, icon);
	return ret_ok;
}


static int
match_file (const char *pattern, 
	    void       *icon,
	    void       *param_file)
{
	return (match (pattern, (char *)param_file));
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
	if (icons->default_icon.len > 0) {
		*icon_ret = icons->default_icon.buf;
	}

	return ret_ok;
}


static ret_t 
add_file (char *file, void *data)
{
	cherokee_icons_t  *icons = ((void **)data)[0];
	cherokee_buffer_t *key   = ((void **)data)[1];
	
	TRACE(ENTRIES, "Adding file icon '%s' -> '%s'\n", key->buf, file);

	return cherokee_icons_add_file (icons, key, file);
}

static ret_t 
add_suffix (char *file, void *data)
{
	cherokee_icons_t  *icons = ((void **)data)[0];
	cherokee_buffer_t *key   = ((void **)data)[1];

	TRACE(ENTRIES, "Adding suffix icon '%s' -> '%s'\n", key->buf, file);
	
	return cherokee_icons_add_suffix (icons, key, file);
}


static ret_t 
configure_file (cherokee_config_node_t *config, void *data)
{
	ret_t  ret;
	void  *params[2];

	params[0] = data;
	params[1] = &config->key;

	ret = cherokee_config_node_read_list (config, NULL, add_file, params);
	if ((ret != ret_ok) && (ret != ret_not_found)) return ret;
	
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
	if ((ret != ret_ok) && (ret != ret_not_found)) return ret;
	
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
		if (ret != ret_ok) return ret;
	}

	ret = cherokee_config_node_get (config, "suffix", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_config_node_while (subconf, configure_suffix, icons);
		if (ret != ret_ok) return ret;
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
