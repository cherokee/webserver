/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega
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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_LIST_H
#define CHEROKEE_LIST_H

#include <cherokee/common.h>


CHEROKEE_BEGIN_DECLS

struct list_entry {
	struct list_entry *next;
	struct list_entry *prev;
};

typedef struct list_entry cherokee_list_t;
typedef struct list_entry cherokee_list_entry_t;


#define LIST(l) ((cherokee_list_t *)(l))

#define INIT_LIST_HEAD(ptr) do {     \
		(ptr)->next = (ptr); \
		(ptr)->prev = (ptr); \
	} while (0)

#define LIST_HEAD_INIT(ptr) { &(ptr), &(ptr) }

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_next_circular(list,item) \
	(((item)->next == (list)) ? (list)->next : (item)->next)

static inline int
cherokee_list_empty (cherokee_list_t *list)
{
	return (list->next == list);
}

static inline void
cherokee_list_add (cherokee_list_t *new_entry, cherokee_list_t *head)
{
	new_entry->next  = head->next;
	new_entry->prev  = head;
	head->next->prev = new_entry;
	head->next       = new_entry;
}

static inline void
cherokee_list_add_tail (cherokee_list_t *new_entry, cherokee_list_t *head)
{
	new_entry->next  = head;
	new_entry->prev  = head->prev;
	head->prev->next = new_entry;
	head->prev       = new_entry;
}

static inline void
cherokee_list_del (cherokee_list_t *entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

static inline void
cherokee_list_reparent (cherokee_list_t *list, cherokee_list_t *new_entry)
{
	if (cherokee_list_empty(list))
		return;

	new_entry->next = list->next;
	new_entry->prev = list->prev;
	new_entry->prev->next = new_entry;
	new_entry->next->prev = new_entry;
}

void  cherokee_list_sort    (cherokee_list_t *head, int (*cmp)(cherokee_list_t *a, cherokee_list_t *b));
ret_t cherokee_list_get_len (cherokee_list_t *head, size_t *len);


/* Methods for non list elements
 */

typedef void (*cherokee_list_free_func) (void *);

typedef struct {
	cherokee_list_entry_t  list;
	void                  *info;
} cherokee_list_item_t;

#define LIST_ITEM(i)      ((cherokee_list_item_t *)(i))
#define LIST_ITEM_INFO(i) (LIST_ITEM(i)->info)

ret_t cherokee_list_add_content              (cherokee_list_t *head, void *item);
ret_t cherokee_list_add_tail_content         (cherokee_list_t *head, void *item);
ret_t cherokee_list_invert                   (cherokee_list_t *head);

ret_t cherokee_list_content_free             (cherokee_list_t *head, cherokee_list_free_func free_func);
ret_t cherokee_list_content_free_item        (cherokee_list_t *head, cherokee_list_free_func free_func);
ret_t cherokee_list_content_free_item_simple (cherokee_list_t *head);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_LIST_H */
