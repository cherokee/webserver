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
#include "table.h"
#include "table-protected.h"

#include <string.h>



typedef struct {
	char *key;
	void *value;
} item_t;


static int
equal (const void *avl_a, const void *avl_b, void *avl_param)
{
	avl_param = avl_param;

	return strcmp(((item_t *)avl_a)->key, ((item_t *)avl_b)->key);
}

static int
equal_case (const void *avl_a, const void *avl_b, void *avl_param)
{
	avl_param = avl_param;

	return strcasecmp(((item_t *)avl_a)->key, ((item_t *)avl_b)->key);
}


static inline void
del_item (void *avl_item, void *avl_param)
{
	free (((item_t *)avl_item)->key);
	free ((item_t *)avl_item);
}


ret_t
cherokee_table_new (cherokee_table_t **tab)
{
	ret_t ret;

	CHEROKEE_NEW_STRUCT(n, table);
	
	ret = cherokee_table_init (n);
	if (unlikely(ret < ret_ok)) return ret;
	
	*tab = n;
	return ret_ok;
}


ret_t
cherokee_table_init (cherokee_table_t *tab)
{
	tab->tree = avl_create (equal, NULL, NULL);
	if (tab->tree == NULL) {
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_table_init_case (cherokee_table_t *tab)
{
	tab->tree = avl_create (equal_case, NULL, NULL);
	if (tab->tree == NULL) {
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_table_free (cherokee_table_t *tab)
{
	cherokee_table_clean (tab);

	free (tab);
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
cherokee_table_mrproper (cherokee_table_t *tab) {
	if (tab->tree) {
		avl_destroy (tab->tree, del_item);
		tab->tree = NULL;
	}
	return ret_ok;
}


ret_t
cherokee_table_mrproper2 (cherokee_table_t *tab, cherokee_table_free_item_t free_func) 
{
	struct avl_traverser  trav;
	item_t               *item;

	/* We've to visit all the nodes of the tree
	 * to free all the 'value' entries
	 */
	avl_t_init (&trav, tab->tree);

	item = (item_t *) avl_t_first (&trav, tab->tree);
	if (item != NULL) {
		free_func (item->value);
	}
	while ((item = avl_t_next(&trav)) != NULL) {
		free_func (item->value);
	}

	avl_destroy (tab->tree, del_item);
	tab->tree = NULL;

	return ret_ok;
}


ret_t
cherokee_table_clean (cherokee_table_t *tab)
{
	cherokee_table_mrproper (tab);
	return cherokee_table_init (tab);
}


ret_t 
cherokee_table_clean2 (cherokee_table_t  *tab, cherokee_table_free_item_t free_func)
{
	if (tab->tree == NULL) {
		return ret_error;
	}

	cherokee_table_mrproper2 (tab, free_func);
	return cherokee_table_init(tab);
}


ret_t
cherokee_table_add (cherokee_table_t *tab, char *key, void *value)
{
	item_t *n = (item_t *)malloc(sizeof(item_t));

	n->key   = strdup (key);
	n->value = value;
	
	avl_insert (tab->tree, n);
	
	return ret_ok;
}

void *
cherokee_table_get_val (cherokee_table_t *tab, char *key)
{
	item_t  n;
	item_t *found;
	
	n.key = key;	
	found = avl_find (tab->tree, &n);
	
	if (found) {
		return found->value;
	}
	
	return NULL;
}

ret_t
cherokee_table_get (cherokee_table_t *tab, char *key, void **ret_val)
{
	item_t  n;
	item_t *found;
	
	n.key = key;	
	found = avl_find (tab->tree, &n);
	
	if (found == NULL) {
		return ret_not_found;
	}

	*ret_val = found->value;	
	return ret_ok;
}


ret_t
cherokee_table_del (cherokee_table_t *tab, char *key, void **val)
{
	item_t  n;
	item_t *found;

	n.key = key;
	found = avl_find (tab->tree, &n);

	if (found == NULL)
		return ret_not_found;
		
	if (val != NULL)
		*val = found->value;

	avl_delete (tab->tree, found);
	return ret_ok;
}


ret_t 
cherokee_table_len (cherokee_table_t *tab, size_t *len)
{
	*len = avl_count(tab->tree);
	return ret_ok;
}


ret_t 
cherokee_table_foreach (cherokee_table_t *tab, cherokee_table_foreach_func_t func)
{
	struct avl_traverser  trav;
	item_t               *item;

	if (tab->tree == NULL) {
		return ret_ok;
	}

	avl_t_init (&trav, tab->tree);

	item = (item_t *) avl_t_first (&trav, tab->tree);
	if (item != NULL) {
#if 0
		printf ("for each: table %p key='%s'\n", tab, item->key);
#endif
		func (item->key, item->value);
	}

	while ((item = avl_t_next(&trav)) != NULL) {
#if 0
		printf ("for each: table %p key='%s'\n", tab, item->key);
#endif
		func (item->key, item->value);
	}

	return ret_ok;
}


ret_t 
cherokee_table_while (cherokee_table_t *tab, cherokee_table_while_func_t func, void *param, char **key, void **value)
{
	struct avl_traverser  trav;
	item_t               *item;
	int                   ret;

	if (tab->tree == NULL) {
		return ret_ok;
	}

	avl_t_init (&trav, tab->tree);

	item = (item_t *) avl_t_first (&trav, tab->tree);
	if (item != NULL) {
		ret = func (item->key, item->value, param);
		if (ret == 0) goto found;
	}

	while ((item = avl_t_next(&trav)) != NULL) {
		ret = func (item->key, item->value, param);
		if (ret == 0) goto found;
	}

	return ret_not_found;

found:
	if (key != NULL) {
		*key   = item->key;
	}
	
	if (value != NULL) {
		*value = item->value;
	}
	return ret_ok;
}


ret_t 
cherokee_table_clean_up (cherokee_table_t *tab, cherokee_table_while_func_t func, void *param)
{
	struct avl_traverser  trav;
	item_t               *item;
	int                   ret;

	if (tab->tree == NULL) {
		return ret_ok;
	}

	avl_t_init (&trav, tab->tree);

	item = (item_t *) avl_t_first (&trav, tab->tree);
	if (item != NULL) {
		ret = func (item->key, item->value, param);
		if (ret) {
			avl_delete (tab->tree, item);
		}
	}
	
	while ((item = avl_t_next(&trav)) != NULL) {
		ret = func (item->key, item->value, param);
		if (ret) {
			avl_delete (tab->tree, item);
		}
	}

	return ret_ok;
}
