/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

#include "common-internal.h"
#include "table.h"

ret_t
cherokee_table_init (cherokee_table_t *tab)
{
	ret_t ret;

	ret = cherokee_avl_init (&tab->avl);
	if (unlikely (ret != ret_ok)) return ret;

	tab->case_sensitive = false;
	return ret_ok;
}

ret_t
cherokee_table_init_case (cherokee_table_t *tab)
{
	ret_t ret;

	ret = cherokee_avl_init (&tab->avl);
	if (unlikely (ret != ret_ok)) return ret;

	tab->case_sensitive = true;
	return ret_ok;
}


ret_t
cherokee_table_free2 (cherokee_table_t *tab, cherokee_table_free_item_t func)
{
	cherokee_table_mrproper2 (tab, func);
	free(tab);
	return ret_ok;
}


ret_t
cherokee_table_clean (cherokee_table_t *tab)
{
	return cherokee_table_clean2 (tab, NULL);
}

static int
free_entry (const char *key, void *value, void *param)
{
	cherokee_table_free_item_t func = param;
	func (value);
	return 0;
}


ret_t
cherokee_table_clean2 (cherokee_table_t *tab, cherokee_table_free_item_t func)
{
	return cherokee_avl_while (&tab->avl, free_entry, func, NULL, NULL);
}


ret_t
cherokee_table_mrproper (cherokee_table_t *tab)
{
	return cherokee_table_clean (tab);
}


ret_t
cherokee_table_mrproper2 (cherokee_table_t *tab, cherokee_table_free_item_t func)
{
	return cherokee_table_clean2 (tab, func);
}


CHEROKEE_ADD_FUNC_NEW(table);
CHEROKEE_ADD_FUNC_FREE(table);


ret_t
cherokee_table_add (cherokee_table_t *tab, char *key, void *value)
{
	cherokee_buffer_t tmp;

	cherokee_buffer_fake (&tmp, key, strlen(key));
	return cherokee_avl_add (&tab->avl, &tmp, value);
}


ret_t
cherokee_table_get (cherokee_table_t *tab, char *key, void **val)
{
	cherokee_buffer_t tmp;

	cherokee_buffer_fake (&tmp, key, strlen(key));
	return cherokee_avl_get (&tab->avl, &tmp, val);
}


ret_t
cherokee_table_del (cherokee_table_t *tab, char *key, void **val)
{
	cherokee_buffer_t tmp;

	cherokee_buffer_fake (&tmp, key, strlen(key));
	return cherokee_avl_get (&tab->avl, &tmp, val);
}


ret_t
cherokee_table_len (cherokee_table_t *tab, size_t *len)
{
	return cherokee_avl_len (&tab->avl, len);
}


static ret_t
each_func_avl_to_table (cherokee_buffer_t *key, void *value, void *param)
{
	// To be removed
	cherokee_table_foreach_func_t func = param;
	func (key->buf, value);
	return ret_ok;
}

ret_t
cherokee_table_foreach (cherokee_table_t *tab, cherokee_table_foreach_func_t func)
{
	return cherokee_avl_while (&tab->avl, each_func_avl_to_table, func, NULL, NULL);
}


ret_t
while_func_avl_to_table (cherokee_buffer_t *key, void *value, void *param)
{
	// To be removed
	int                          re;
	void                       **params   = param;
	cherokee_table_while_func_t  func     = params[0];
	void                        *data     = params[1];

	re = func (key->buf, value, data);
	if (re != 0) return ret_not_found;

	return ret_ok;
}


ret_t
cherokee_table_while (cherokee_table_t *tab, cherokee_table_while_func_t func, void *param, char **key, void **value)
{
	ret_t              ret;
	cherokee_buffer_t *key_buf  = NULL;
	void              *params[] = { func, param };

	ret = cherokee_avl_while (&tab->avl, while_func_avl_to_table, (void *)params, &key_buf, value);
	if (key && key_buf)
		*key = key_buf->buf;

	return ret;
}
