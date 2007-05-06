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
#include "avl.h"

#include <string.h>
#include <strings.h>


static int
equal (void *avl_param, void *key, void *val)
{
/*	printf ("equal (%s, %s) = %d\n", key, val, strcmp((const char *)key, (const char *)val)); */
	return strcmp((const char *)key, (const char *)val);
}

static int
equal_case (void *avl_param, void *key, void *val)
{
	return strcasecmp((const char *)key, (const char *)val);
}


ret_t
cherokee_table_new (cherokee_table_t **tab)
{
	*tab = avl_new_avl_tree (equal, NULL);
	if (unlikely (*tab == NULL)) return ret_nomem;

	return ret_ok;
}


ret_t
cherokee_table_init (cherokee_table_t *tab)
{
	avl_node * root = avl_new_avl_node(NULL, NULL, NULL);
	if (unlikely (root == NULL)) 
		return ret_nomem;

	tab->root        = root;
	tab->length      = 0;
	tab->compare_fun = equal;
	tab->compare_arg = NULL;
	
	return ret_ok;
}


ret_t
cherokee_table_init_case (cherokee_table_t *tab)
{
	ret_t ret;

	ret = cherokee_table_init (tab);
	if (unlikely (ret != ret_ok)) return ret;

	tab->compare_fun = equal_case;

	return ret_ok;
}


ret_t
cherokee_table_mrproper2 (cherokee_table_t *tab, cherokee_table_free_item_t free_func) 
{
	avl_free_avl_mrproper (tab, 
			       (avl_free_key_fun_type)free, 
			       (avl_free_key_fun_type)free_func);
	return ret_ok;
}

ret_t 
cherokee_table_free2 (cherokee_table_t  *tab, cherokee_table_free_item_t free_func)
{
	cherokee_table_mrproper2 (tab, free_func);

	free (tab);
	return ret_ok;	
}


ret_t
cherokee_table_free (cherokee_table_t *tab)
{
	return cherokee_table_free2 (tab, free);
}

ret_t
cherokee_table_mrproper (cherokee_table_t *tab) 
{
	return cherokee_table_mrproper2 (tab, NULL);
}


ret_t
cherokee_table_clean (cherokee_table_t *tab)
{
	cherokee_table_mrproper2 (tab, NULL);
	return cherokee_table_init (tab);
}


ret_t 
cherokee_table_clean2 (cherokee_table_t *tab, cherokee_table_free_item_t free_func)
{
	if (unlikely (tab == NULL)) 
		return ret_error;

	cherokee_table_mrproper2 (tab, free_func);
	return cherokee_table_init(tab);
}


ret_t
cherokee_table_add (cherokee_table_t *tab, char *key, void *value)
{
	int          re;
	unsigned int index;

	re = avl_insert_by_key (tab, strdup(key), value, &index);
	if (unlikely (re != 0)) {
		switch (re) {
		case -2:
			return ret_deny;
		default:
			return ret_error;
		}
	}

	return ret_ok;
}

void *
cherokee_table_get_val (cherokee_table_t *tab, char *key)
{
	ret_t  ret;
	void  *val = NULL;

	ret = cherokee_table_get (tab, key, &val);	
	if (unlikely (ret != ret_ok)) return NULL;

	return val;
}

ret_t
cherokee_table_get (cherokee_table_t *tab, char *key, void **ret_val)
{
	int re; 

	re = avl_get_item_by_key (tab, key, ret_val);
	if (unlikely (re != 0)) return ret_not_found;

	return ret_ok;
}


ret_t
cherokee_table_del (cherokee_table_t *tab, char *key, void **val)
{
	int re;

	re = avl_remove_by_key (tab, key, (avl_free_key_fun_type)free, val);
	if (unlikely (re != 0)) return ret_error;

	return ret_ok;
}


ret_t 
cherokee_table_len (cherokee_table_t *tab, size_t *len)
{
	*len = tab->length;
	return ret_ok;
}


static int
foreach_wrapper (void *key, void *val, void *iter_arg)
{
	cherokee_table_foreach_func_t func = (cherokee_table_foreach_func_t)iter_arg;

	func (key, val);
	return 0;
}


ret_t 
cherokee_table_foreach (cherokee_table_t *tab, cherokee_table_foreach_func_t func)
{
	int re;

	re = avl_iterate_inorder (tab, foreach_wrapper, (void *)func, NULL, NULL);
	if (unlikely (re != 0)) return ret_error;

	return ret_ok;
}


ret_t 
cherokee_table_while (cherokee_table_t *tab, cherokee_table_while_func_t func, void *param, char **key, void **value)
{
	int re;

	re = avl_iterate_inorder (tab, (avl_iter_fun_type)func, param, (void **)key, value);
	if (re == 0) return ret_not_found;

	return ret_ok;
}
