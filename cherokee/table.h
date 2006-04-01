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

#ifndef CHEROKEE_TABLE_H
#define CHEROKEE_TABLE_H

#include <cherokee/common.h>


CHEROKEE_BEGIN_DECLS

typedef void (* cherokee_table_free_item_t)    (void *item);
typedef void (* cherokee_table_foreach_func_t) (const char *key, void *value);
typedef int  (* cherokee_table_while_func_t)   (const char *key, void *value, void *param);


typedef struct cherokee_table cherokee_table_t;
#define TABLE(x) ((cherokee_table_t *)(x))


ret_t cherokee_table_new       (cherokee_table_t **tab);
ret_t cherokee_table_init      (cherokee_table_t  *tab);
ret_t cherokee_table_init_case (cherokee_table_t  *tab);

ret_t cherokee_table_free      (cherokee_table_t  *tab);
ret_t cherokee_table_free2     (cherokee_table_t  *tab, cherokee_table_free_item_t func);
ret_t cherokee_table_clean     (cherokee_table_t  *tab);
ret_t cherokee_table_clean2    (cherokee_table_t  *tab, cherokee_table_free_item_t func);
ret_t cherokee_table_mrproper  (cherokee_table_t  *tab);
ret_t cherokee_table_mrproper2 (cherokee_table_t  *tab, cherokee_table_free_item_t func);

ret_t cherokee_table_add       (cherokee_table_t *tab, char *key, void *value);
void* cherokee_table_get_val   (cherokee_table_t *tab, char *key);
ret_t cherokee_table_get       (cherokee_table_t *tab, char *key, void **val);
ret_t cherokee_table_del       (cherokee_table_t *tab, char *key, void **val);
ret_t cherokee_table_len       (cherokee_table_t *tab, size_t *len);

ret_t cherokee_table_foreach   (cherokee_table_t *tab, cherokee_table_foreach_func_t func);
ret_t cherokee_table_while     (cherokee_table_t *tab, cherokee_table_while_func_t func, void *param, char **key, void **value);
ret_t cherokee_table_clean_up  (cherokee_table_t *tab, cherokee_table_while_func_t func, void *param);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_TABLE_H */
