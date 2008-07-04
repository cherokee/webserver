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
#include "list.h"

#include <stdlib.h>

ret_t
cherokee_list_get_len (cherokee_list_t *head, size_t *len)
{
	cherokee_list_t *i;		
	cuint_t          n = 0;
	
	list_for_each (i, head) 
		n++;

	*len = n;
	return ret_ok;
}


void 
cherokee_list_sort (cherokee_list_t *head, int (*cmp)(cherokee_list_t *a, cherokee_list_t *b))
{
	cherokee_list_t *p, *q, *e, *list, *tail, *oldhead;
	int insize, nmerges, psize, qsize, i;
	
	list = head->next;
	cherokee_list_del(head);
	insize = 1;
	for (;;) {
		p = oldhead = list;
		list = tail = NULL;
		nmerges = 0;

		while (p) {
			nmerges++;
			q = p;
			psize = 0;
			for (i = 0; i < insize; i++) {
				psize++;
				q = q->next == oldhead ? NULL : q->next;
				if (!q)
					break;
			}

			qsize = insize;
			while (psize > 0 || (qsize > 0 && q)) {
				if (!psize) {
					e = q;
					q = q->next;
					qsize--;
					if (q == oldhead)
						q = NULL;
				} else if (!qsize || !q) {
					e = p;
					p = p->next;
					psize--;
					if (p == oldhead)
						p = NULL;
				} else if (cmp(p, q) <= 0) {
					e = p;
					p = p->next;
					psize--;
					if (p == oldhead)
						p = NULL;
				} else {
					e = q;
					q = q->next;
					qsize--;
					if (q == oldhead)
						q = NULL;
				}
				if (tail)
					tail->next = e;
				else
					list = e;
				e->prev = tail;
				tail = e;
			}
			p = q;
		}

		tail->next = list;
		list->prev = tail;

		if (nmerges <= 1)
			break;

		insize *= 2;
	}

	head->next = list;
	head->prev = list->prev;
	list->prev->next = head;
	list->prev = head;
}



ret_t 
cherokee_list_add_content (cherokee_list_t *head, void *item)
{
	CHEROKEE_NEW_STRUCT(n,list_item);
	
	/* Init
	 */
	INIT_LIST_HEAD (LIST(n));
	n->info = item;
	
	/* Add to list
	 */
	cherokee_list_add (LIST(n), head);
	
	return ret_ok;
}


ret_t 
cherokee_list_add_tail_content (cherokee_list_t *head, void *item)
{
	CHEROKEE_NEW_STRUCT(n,list_item);
	
	/* Init
	 */
	INIT_LIST_HEAD (LIST(n));
	n->info = item;
	
	/* Add to list
	 */
	cherokee_list_add_tail (LIST(n), head);
	
	return ret_ok;
}


ret_t 
cherokee_list_content_free (cherokee_list_t *head, cherokee_list_free_func free_func)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, head) {
		cherokee_list_content_free_item (i, free_func);
	}
	
	INIT_LIST_HEAD(head);
	
	return ret_ok;
}


ret_t 
cherokee_list_content_free_item (cherokee_list_t *head, cherokee_list_free_func free_func)
{
	cherokee_list_del (head);
	
	if ((free_func != NULL) && (LIST_ITEM(head)->info)) {
		free_func (LIST_ITEM(head)->info);
	}
	
	free (head);
	return ret_ok;
}


ret_t 
cherokee_list_content_free_item_simple (cherokee_list_t *head)
{
	cherokee_list_del (head);
	
	if (LIST_ITEM(head)->info) {
		free (LIST_ITEM(head)->info);
	}
	
	free (head);
	return ret_ok;	
}
