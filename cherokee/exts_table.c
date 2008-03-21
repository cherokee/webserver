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
#include "exts_table.h"
#include "util.h"

#define ENTRIES "exts"


ret_t 
cherokee_exts_table_init (cherokee_exts_table_t *et)
{
	ret_t ret;

	ret = cherokee_avl_init(&et->avl);
	if (unlikely (ret != ret_ok)) return ret;

	INIT_LIST_HEAD(&et->list);

	return ret_ok;
}


ret_t 
cherokee_exts_table_mrproper (cherokee_exts_table_t *et)
{
	cherokee_list_content_free (&et->list, (cherokee_list_free_func) cherokee_config_entry_free);
	cherokee_avl_mrproper (&et->avl, NULL);
	return ret_ok;
}

ret_t 
cherokee_exts_table_get (cherokee_exts_table_t *et, cherokee_buffer_t *requested_url, cherokee_config_entry_t *plugin_entry)
{
	ret_t                    ret;
	char                    *dot;
	cherokee_config_entry_t *entry;

	dot = strrchr (requested_url->buf, '.');
	if (dot == NULL) return ret_not_found;

	ret = cherokee_avl_get_ptr (&et->avl, dot+1, (void **)&entry);
	if (ret != ret_ok) return ret;

	TRACE (ENTRIES, "Match with \"%s\"\n", dot+1);
	
	cherokee_config_entry_complete (plugin_entry, entry, false);
	return ret_ok;
}


ret_t 
cherokee_exts_table_add  (cherokee_exts_table_t *et, char *ext, cherokee_config_entry_t *plugin_entry)
{
	cherokee_list_t   *i;
	cherokee_boolean_t found = false;

	/* Each plugin entry has to be added to the list only once
	 */
	list_for_each (i, &et->list) {
		found |= (LIST_ITEM_INFO(i) == plugin_entry);
	}

	if (!found) {
		cherokee_list_add_content (&et->list, plugin_entry);
	}

	/* Add to the table. It is ok if many entries point to the same
	 * plugin entry object.
	 */
	TRACE (ENTRIES, "ADD: et %p avl %p ext %s\n", et, &et->avl, ext);

	return cherokee_avl_add_ptr (&et->avl, ext, plugin_entry);
}

ret_t 
cherokee_exts_table_has (cherokee_exts_table_t *et, char *ext)
{
	void *foo;
	return cherokee_avl_get_ptr (&et->avl, ext, &foo);
}
