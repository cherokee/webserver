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
#include "list_ext.h"


ret_t 
cherokee_list_add (struct list_head *head, void *item)
{
	   CHEROKEE_NEW_STRUCT(n,list_item);

	   /* Init
	    */
	   INIT_LIST_HEAD((list_t*)n);
	   n->info = item;

	   /* Add to list
	    */
	   list_add ((list_t *)n, head);

	   return ret_ok;
}


ret_t 
cherokee_list_add_tail (struct list_head *head, void *item)
{
	   CHEROKEE_NEW_STRUCT(n,list_item);

	   /* Init
	    */
	   INIT_LIST_HEAD((list_t*)n);
	   n->info = item;

	   /* Add to list
	    */
	   list_add_tail ((list_t *)n, head);

	   return ret_ok;
}


ret_t 
cherokee_list_free (struct list_head *head, void (*free_func) (void *))
{
	   list_t *i, *tmp;

	   list_for_each_safe (i, tmp, head) {
		   cherokee_list_free_item (i, free_func);
	   }

	   INIT_LIST_HEAD(head);

	   return ret_ok;
}


ret_t 
cherokee_list_free_item (struct list_head *head, void (*free_func) (void *))
{
	list_del (head);
	
	if ((free_func != NULL) && (LIST_ITEM(head)->info)) {
		free_func (LIST_ITEM(head)->info);
	}
	
	free (head);
	return ret_ok;
}


ret_t 
cherokee_list_free_item_simple (struct list_head *head)
{
	list_del (head);
	
	if (LIST_ITEM(head)->info) {
		free (LIST_ITEM(head)->info);
	}
	
	free (head);
	return ret_ok;	
}
