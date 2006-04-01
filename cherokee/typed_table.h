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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_TYPED_TABLE_H
#define CHEROKEE_TYPED_TABLE_H

#include <cherokee/common.h>
#include <cherokee/table.h>
#include <cherokee/list.h>

CHEROKEE_BEGIN_DECLS

typedef enum {
	typed_none,
	typed_int,
	typed_str,
	typed_data,
	typed_list
} cherokee_typed_table_types_t;


typedef void (* cherokee_typed_free_func_t) (void *);

ret_t cherokee_typed_table_clean    (cherokee_table_t *table);
ret_t cherokee_typed_table_free     (cherokee_table_t *table);

ret_t cherokee_typed_table_add_data (cherokee_table_t *table, char *index, void *data, cherokee_typed_free_func_t free);
ret_t cherokee_typed_table_add_list (cherokee_table_t *table, char *index, list_t *list, cherokee_typed_free_func_t free);
ret_t cherokee_typed_table_add_int  (cherokee_table_t *table, char *index, cuint_t num);
ret_t cherokee_typed_table_add_str  (cherokee_table_t *table, char *index, char *str);

ret_t cherokee_typed_table_get_data (cherokee_table_t *table, char *index, void **data);
ret_t cherokee_typed_table_get_list (cherokee_table_t *table, char *index, list_t **list);
ret_t cherokee_typed_table_get_int  (cherokee_table_t *table, char *index, cuint_t *num);
ret_t cherokee_typed_table_get_str  (cherokee_table_t *table, char *index, char **str);

ret_t cherokee_typed_table_update_data (cherokee_table_t *table, char *index, void *data);
ret_t cherokee_typed_table_update_list (cherokee_table_t *table, char *index, list_t *list);
ret_t cherokee_typed_table_update_int  (cherokee_table_t *table, char *index, cuint_t num);
ret_t cherokee_typed_table_update_str  (cherokee_table_t *table, char *index, char *str);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_TYPED_TABLE_H */
