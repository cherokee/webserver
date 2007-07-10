/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2007 Alvaro Lopez Ortega
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
#include "table.h"

#define AVL_HANDLE     void *
#define AVL_KEY        cherokee_buffer_t *
#define AVL_MAX_DEPTH  45

#include "avl_if.h"


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
	cherokee_table_clean (tab);
	cherokee_table_free (tab);
	return ret_ok; 
}


ret_t 
cherokee_table_clean (cherokee_table_t *tab) 
{ 
	return cherokee_table_clean2 (tab, NULL);
}


ret_t 
cherokee_table_clean2 (cherokee_table_t *tab, cherokee_table_free_item_t func) 
{ 
	// TODO
	return ret_ok; 
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
	ret_t             ret;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	cherokee_buffer_add (&tmp, key, strlen(key));
	ret = cherokee_avl_add (&tab->avl, &tmp, value);
	cherokee_buffer_mrproper (&tmp);

	return ret;
}


void* 
cherokee_table_get_val (cherokee_table_t *tab, char *key)
{
	void  *re = NULL;

	cherokee_table_get (tab, key, &re);
	return re;
}


ret_t 
cherokee_table_get (cherokee_table_t *tab, char *key, void **val)
{
	ret_t ret;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	cherokee_buffer_add (&tmp, key, strlen(key));
	ret = cherokee_avl_get (&tab->avl, &tmp, val);
	cherokee_buffer_mrproper (&tmp);

	return ret;
}


ret_t 
cherokee_table_del (cherokee_table_t *tab, char *key, void **val)
{
	ret_t ret;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	cherokee_buffer_add (&tmp, key, strlen(key));
	ret = cherokee_avl_get (&tab->avl, &tmp, val);
	cherokee_buffer_mrproper (&tmp);

	return ret;
}


ret_t 
cherokee_table_len (cherokee_table_t *tab, size_t *len)
{
	return cherokee_avl_len (&tab->avl, len);
}



ret_t 
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
