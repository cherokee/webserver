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

#ifndef CHEROKEE_LIST_EXT_H
#define CHEROKEE_LIST_EXT_H

#include "common.h"
#include "list.h"

typedef void (*cherokee_list_free_func) (void *);

typedef struct {
	   list_t  list;
	   void   *info;
} cherokee_list_item_t;

#define LIST_ITEM(i)      ((cherokee_list_item_t *)(i))
#define LIST_ITEM_INFO(i) (LIST_ITEM(i)->info)


ret_t cherokee_list_add              (list_t *head, void *item);
ret_t cherokee_list_add_tail         (list_t *head, void *item);

ret_t cherokee_list_free             (list_t *head, cherokee_list_free_func free_func);
ret_t cherokee_list_free_item        (list_t *head, cherokee_list_free_func free_func);
ret_t cherokee_list_free_item_simple (list_t *head);

#endif /* CHEROKEE_LIST_EXT_H */
