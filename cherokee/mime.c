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

#include <ctype.h>

#include "mime.h"
#include "mime-protected.h"
#include "util.h"

#define ENTRIES "mime"


ret_t 
cherokee_mime_new (cherokee_mime_t **mime)
{
	CHEROKEE_NEW_STRUCT(n, mime);

	cherokee_avl_init (&n->ext_table);
	INIT_LIST_HEAD(&n->entry_list);
	   
	/* Return the object
	 */
	*mime = n;
	return ret_ok;
}


ret_t
cherokee_mime_free (cherokee_mime_t *mime)
{
	cherokee_list_t *i, *tmp;

	if (mime == NULL)
		return ret_ok;

	cherokee_avl_mrproper (&mime->ext_table, NULL);

	list_for_each_safe (i, tmp, &mime->entry_list) {
		cherokee_list_del (i);
		cherokee_mime_entry_free (MIME_ENTRY(i));
	}

	free (mime);
	return ret_ok;
}


static ret_t 
get_by_mime (cherokee_mime_t *mime, cherokee_buffer_t *type, cherokee_mime_entry_t **entry)
{
	ret_t            ret;
	cherokee_list_t *i;

	list_for_each (i, &mime->entry_list) {
		cherokee_buffer_t *itype;

		ret = cherokee_mime_entry_get_type (MIME_ENTRY(i), &itype);
		if (ret != ret_ok) continue;

		if (cherokee_buffer_cmp_buf (type, itype) == 0) {
			*entry = MIME_ENTRY(i);
			return ret_ok;
		}
	}

	return ret_not_found;
}


static ret_t 
get_by_mime_or_new (cherokee_mime_t *mime, cherokee_buffer_t *type, cherokee_mime_entry_t **entry)
{
	ret_t ret;

	ret = get_by_mime (mime, type, entry);
	if (ret == ret_ok)
		return ret_ok;

	ret = cherokee_mime_entry_new (entry); 
	if (ret != ret_ok) return ret;

	ret = cherokee_mime_entry_set_type (*entry, type);
	if (ret != ret_ok) return ret;

	cherokee_list_add (LIST(*entry), &mime->entry_list);

	TRACE(ENTRIES, "Type set '%s'\n", type->buf);
	return ret_ok;
}


static ret_t
add_extension (char *val, void *data)
{
	ret_t                  ret;
	cherokee_mime_entry_t *entry = NULL;
	cherokee_mime_t       *mime  = ((void **)data)[0];
	cherokee_buffer_t     *type  = ((void **)data)[1];

	/* Get the mime entry object
	 */
	ret = get_by_mime_or_new (mime, type, &entry);
	if (ret != ret_ok) return ret;
	
	/* Link to it by the extension
	 */
	TRACE(ENTRIES, "'%s' adding extension: '%s'\n", type->buf, val);
	cherokee_avl_add_ptr (&mime->ext_table, (const char *)val, entry); 
	
	return ret_ok;
}

static ret_t
set_maxage (cherokee_mime_t *mime, cherokee_buffer_t *type, int maxage)
{
	ret_t                  ret;
	cherokee_mime_entry_t *entry = NULL;

	/* Get the mime entry object
	 */
	ret = get_by_mime_or_new (mime, type, &entry);
	if (ret != ret_ok) return ret;

	/* Setting max-age
	 */
	TRACE(ENTRIES, "'%s' setting max-age: %d\n", type->buf, maxage);
	cherokee_mime_entry_set_maxage (entry, maxage);
		
	return ret_ok;
}


static ret_t 
configure_mime (cherokee_config_node_t *config, void *data)
{
	ret_t                   ret;
	cint_t                  maxage;
	cherokee_config_node_t *subconf;
	cherokee_mime_t        *mime     = MIME(data);
	void                   *params[] = {mime, &config->key};

	ret = cherokee_config_node_get (config, "extensions", &subconf);
	if (ret == ret_ok) {
		cherokee_config_node_read_list (subconf, NULL, add_extension, params);
	}

	ret = cherokee_config_node_read_int (config, "max-age", &maxage);
	if (ret == ret_ok) {
		ret = set_maxage (mime, &config->key, maxage);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}


ret_t 
cherokee_mime_configure (cherokee_mime_t *mime, cherokee_config_node_t *config)
{
	ret_t ret;

	ret = cherokee_config_node_while (config, configure_mime, mime);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


ret_t 
cherokee_mime_get_by_suffix (cherokee_mime_t *mime, char *suffix, cherokee_mime_entry_t **entry)
{
	return cherokee_avl_get_ptr (&mime->ext_table, suffix, (void **)entry);
}

