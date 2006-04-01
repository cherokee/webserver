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
#include "typed_table.h"


typedef struct {
	union {
		cuint_t  num;
		char    *str;
		void    *data;
		list_t   list;
	} data;
	
	cherokee_typed_table_types_t type;
	cherokee_typed_free_func_t   free;
} item_t;


static item_t *
new_item (void) 
{
	item_t *n;

	n = (item_t *) malloc (sizeof(item_t));
	if (unlikely (n == NULL)) 
		return NULL;

	n->data.num  = 0;
	n->data.str  = NULL;
	n->data.data = NULL;

	INIT_LIST_HEAD(&n->data.list);	

	n->type = typed_none;
	n->free = NULL;

	return n;
}


static void
free_item (item_t *n)
{
	list_t *i, *tmp;

	if (n == NULL)
		return;

	switch (n->type) {
	case typed_none:
	case typed_int:
		break;

	case typed_str:
		if (n->data.str != NULL) 
			free (n->data.str);
		break;

	case typed_data:
		if (n->data.data != NULL) {
			if (n->free != NULL) {
				n->free (n->data.data);
			} else {
				free (n->data.data);
			}
		}
		break;

	case typed_list:
		list_for_each_safe (i, tmp, &n->data.list) {
			list_del (i);

			if (n->free != NULL) {
				n->free (i);
			} else {
				free (i);
			}
		}
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	free (n);
}



ret_t 
cherokee_typed_table_add_data (cherokee_table_t *table, char *index, void *data, cherokee_typed_free_func_t free)
{
	item_t *n;

	n = new_item();
	if (unlikely (n == NULL)) return ret_nomem;

	n->type = typed_data;
	n->data.data = data;
	n->free = free;

	return cherokee_table_add (table, index, n);
}


ret_t 
cherokee_typed_table_add_list (cherokee_table_t *table, char *index, list_t *list, cherokee_typed_free_func_t free)
{
	item_t *n;

	n = new_item();
	if (unlikely (n == NULL)) return ret_nomem;

	n->type = typed_list;
	list_reparent (list, &n->data.list);
	n->free = free;

	return cherokee_table_add (table, index, n);
}


ret_t 
cherokee_typed_table_add_int  (cherokee_table_t *table, char *index, cuint_t num)
{
	item_t *n;

	n = new_item();
	if (unlikely (n == NULL)) return ret_nomem;

	n->type = typed_int;
	n->data.num = num;
	return cherokee_table_add (table, index, n);
}


ret_t 
cherokee_typed_table_add_str (cherokee_table_t *table, char *index, char *str)
{
	item_t *n;

	n = new_item();
	if (unlikely (n == NULL)) return ret_nomem;

	n->type = typed_str;
	n->data.str = str;
	return cherokee_table_add (table, index, n);
}


ret_t
cherokee_typed_table_clean (cherokee_table_t *table)
{
	return cherokee_table_clean2 (table, (cherokee_table_free_item_t)free_item);
}


ret_t
cherokee_typed_table_free (cherokee_table_t *table)
{
	return cherokee_table_free2 (table, (cherokee_table_free_item_t)free_item);
}


ret_t 
cherokee_typed_table_get_data (cherokee_table_t *table, char *index, void **data)
{
	ret_t   ret;
	item_t *val;

	ret = cherokee_table_get (table, index, (void **)&val);
	if (ret != ret_ok) return ret;

        if (val->type != typed_data) 
		return ret_error;

	*data = val->data.data;
	return ret_ok;
}


ret_t 
cherokee_typed_table_get_list (cherokee_table_t *table, char *index, list_t **list)
{
	ret_t   ret;
	item_t *val;

	ret = cherokee_table_get (table, index, (void **)&val);
	if (ret != ret_ok) return ret;

        if (val->type != typed_list) 
		return ret_error;
	
	*list = &val->data.list;
	return ret_ok;
}


ret_t 
cherokee_typed_table_get_int (cherokee_table_t *table, char *index, cuint_t *num)
{
	ret_t   ret;
	item_t *val;

	ret = cherokee_table_get (table, index, (void *)&val);
	if (ret != ret_ok) return ret;

        if (val->type != typed_int) 
		return ret_error;

	*num = val->data.num;
	return ret_ok;
}


ret_t 
cherokee_typed_table_get_str (cherokee_table_t *table, char *index, char **str)
{
	ret_t   ret;
	item_t *val;

	ret = cherokee_table_get (table, index, (void **)&val);
	if (ret != ret_ok) return ret;

        if (val->type != typed_str) 
		return ret_error;

	*str = val->data.str;
	return ret_ok;
}


ret_t 
cherokee_typed_table_update_data (cherokee_table_t *table, char *index, void *data)
{
	ret_t   ret;
	item_t *val;

	ret = cherokee_table_get (table, index, (void **)&val);
	if (ret != ret_ok) return ret;

	val->data.data = data;
	return ret_ok;
}


ret_t 
cherokee_typed_table_update_list (cherokee_table_t *table, char *index, list_t *list)
{
	ret_t   ret;
	item_t *val;

	ret = cherokee_table_get (table, index, (void **)&val);
	if (ret != ret_ok) return ret;

	list_reparent (list, &val->data.list);
	return ret_ok;
}


ret_t 
cherokee_typed_table_update_int  (cherokee_table_t *table, char *index, cuint_t num)
{
	ret_t   ret;
	item_t *val;

	ret = cherokee_table_get (table, index, (void **)&val);
	if (ret != ret_ok) return ret;

	val->data.num = num;
	return ret_ok;
}


ret_t 
cherokee_typed_table_update_str  (cherokee_table_t *table, char *index, char *str)
{
	ret_t   ret;
	item_t *val;

	ret = cherokee_table_get (table, index, (void **)&val);
	if (ret != ret_ok) return ret;

	val->data.str = str;
	return ret_ok;
}
