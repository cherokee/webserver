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

#include "dirs_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#include "handler.h"

#define ENTRIES "dirs"


ret_t 
cherokee_dirs_table_new (cherokee_dirs_table_t **pt)
{
	return cherokee_table_new(pt);
}

ret_t 
cherokee_dirs_table_free (cherokee_dirs_table_t *pt)
{
	return cherokee_table_free2 (
		TABLE(pt), 
		(cherokee_table_free_item_t) cherokee_config_entry_free);
}


ret_t 
cherokee_dirs_table_init (cherokee_dirs_table_t *pt)
{
	return cherokee_table_init (pt);
}


ret_t 
cherokee_dirs_table_mrproper (cherokee_dirs_table_t *pt)
{
	return cherokee_table_mrproper2 (
		TABLE(pt), 
		(cherokee_table_free_item_t) cherokee_config_entry_free);
}


ret_t 
cherokee_dirs_table_get (cherokee_dirs_table_t *pt, cherokee_buffer_t *requested_url, 
			 cherokee_config_entry_t *plugin_entry, cherokee_buffer_t *web_directory)
{
	ret_t                    ret;
	char                    *slash;
	cherokee_config_entry_t *entry = NULL;

	cherokee_buffer_add_buffer (web_directory, requested_url);

	do {
		ret = cherokee_table_get (pt, web_directory->buf, (void **)&entry);
		
		/* Found
		 */
		if ((ret == ret_ok) && (entry != NULL))
			goto go_out;
 
		if (web_directory->len <= 1)
			goto go_out;

		/* Modify url for next loop:
		 * Find the last / and finish the string there.
		 */
		slash = strrchr (web_directory->buf, '/');
		if (slash == NULL) goto go_out;

		*slash = '\0';
		web_directory->len -= ((web_directory->buf + web_directory->len) - slash);

#if 0
		printf ("cherokee_dirs_table_get(): requested_url: %s\n", requested_url->buf);
		printf ("cherokee_dirs_table_get(): web_directory: %s\n", web_directory->buf);
		printf ("\n");
#endif

	} while (entry == NULL);


go_out:	
	if (entry != NULL) {
		TRACE (ENTRIES, "Match with \"%s\"\n", web_directory->buf);

		/* Copy everythin into the duplicate
		 */
		cherokee_config_entry_complete (plugin_entry, entry, false);

		/* Inherit from parents
		 */
		cherokee_config_entry_inherit (plugin_entry);
	}
	
#if 0
	printf ("ptable::GET - entry %p\n", entry);
	if (entry) printf ("ptable::GET - entry->handler_properties: %p\n", entry->handler_properties);
	if (entry) printf ("ptable::GET - entry->validator_properties: %p\n", entry->validator_properties);
#endif

	return (entry == NULL) ? ret_not_found : ret_ok;
}


ret_t
cherokee_dirs_table_add (cherokee_dirs_table_t *pt, char *dir, cherokee_config_entry_t *plugin_entry)
{
#if 0
	printf ("cherokee_dirs_table_add(table=%p, dir=\"%s\", entry=%p)\n", pt, dir, plugin_entry);
#endif

	/* Add to "dir <-> plugin_entry" table
	 */
	return cherokee_table_add (TABLE(pt), dir, (void*)plugin_entry);
}


int
relink_func (const char *key_, void *value, void *param)
{
	ret_t                        ret;
	char                        *slash;
	cherokee_buffer_t            key   = CHEROKEE_BUF_INIT;
	cherokee_dirs_table_t       *table = DTABLE(param);
	cherokee_config_entry_t     *entry = CONF_ENTRY(value);

	/* It has to look the the parent of this directory
	 */
	cherokee_buffer_add (&key, (char *)key_, strlen(key_));

	do {
		void *parent = NULL;

		if (cherokee_buffer_is_endding (&key, '/')) {
			cherokee_buffer_drop_endding (&key, 1);
		} else {
			slash = strrchr (key.buf, '/');
			if (slash == NULL) goto out;
	
			slash[1] = '\0';
			key.len -= ((key.buf + key.len) - slash -1);
		}

		ret = cherokee_table_get (table, key.buf, &parent);
		if (ret == ret_ok) {
			entry->parent = parent;
			goto out;
		}

	} while (key.len > 1);
	
out:
	cherokee_buffer_mrproper (&key);
	return true;
}


ret_t 
cherokee_dirs_table_relink (cherokee_dirs_table_t *pt)
{
	return cherokee_table_while (TABLE(pt), relink_func, pt, NULL, NULL);
}
