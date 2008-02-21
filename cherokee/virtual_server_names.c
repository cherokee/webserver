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
#include "virtual_server_names.h"
#include "match.h"


/* Virtual server names
 */

ret_t 
cherokee_vserver_names_add_name (cherokee_vserver_names_t *list, cherokee_buffer_t *name)
{
	ret_t                          ret;
	cherokee_vserver_name_entry_t *entry;

	/* Create a new name object
	 */
	ret = cherokee_vserver_name_entry_new (&entry, name);
	if (ret != ret_ok)
		return ret;

	/* Add the new entry
	 */
	cherokee_list_add (LIST(entry), list);

	return ret_ok;
}


ret_t 
cherokee_vserver_names_find (cherokee_vserver_names_t *list, cherokee_buffer_t *name)
{
	ret_t            ret;
	cherokee_list_t *i;

	list_for_each (i, list) {
		ret = cherokee_vserver_name_entry_match (VSERVER_NAME(i), name);
		if (ret == ret_ok)
			return ret_ok;
	}

	return ret_not_found;
}


ret_t 
cherokee_vserver_names_mrproper (cherokee_vserver_names_t *list)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, list) {
		cherokee_vserver_name_entry_free (VSERVER_NAME(i));
	}

	return ret_ok;
}


/* Virtual server name entry
 */
ret_t 
cherokee_vserver_name_entry_new (cherokee_vserver_name_entry_t **entry, cherokee_buffer_t *name)
{
	CHEROKEE_NEW_STRUCT (n, vserver_name_entry);

	/* It's a list node
	 */
	INIT_LIST_HEAD (&n->node);

	/* Name entry
	 */
	cherokee_buffer_init (&n->name);
	cherokee_buffer_add_buffer (&n->name, name);

	/* Check if the name contains wildcards
	 */
	n->is_wildcard = (strchr (name->buf, '*') || strchr (name->buf, '?'));

	*entry = n;

	return ret_ok;
}


ret_t 
cherokee_vserver_name_entry_match (cherokee_vserver_name_entry_t *entry, cherokee_buffer_t *name)
{
	int re;

	if (entry->is_wildcard) 
		return cherokee_wildcard_match (entry->name.buf, name->buf);

	re = cherokee_buffer_cmp_buf (&entry->name, name); 
	return (re == 0) ? ret_ok : ret_deny;
}


ret_t 
cherokee_vserver_name_entry_free (cherokee_vserver_name_entry_t *entry)
{
	cherokee_buffer_mrproper (&entry->name);

	free (entry);
	return ret_ok;
}
