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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_AVL_H
#define CHEROKEE_AVL_H

#include <cherokee/buffer.h>


CHEROKEE_BEGIN_DECLS

typedef struct cherokee_avl_node cherokee_avl_node_t;

typedef struct {
	cherokee_avl_node_t *root;
	cherokee_boolean_t   case_insensitive;
} cherokee_avl_t;

#define AVL(a) ((cherokee_avl_t *)(a))

typedef ret_t (* cherokee_avl_while_func_t)      (cherokee_buffer_t *key, void *value, void *param);

ret_t cherokee_avl_new       (cherokee_avl_t **avl);
ret_t cherokee_avl_free      (cherokee_avl_t  *avl, cherokee_func_free_t free_func);

ret_t cherokee_avl_init      (cherokee_avl_t  *avl);
ret_t cherokee_avl_mrproper  (cherokee_avl_t  *avl, cherokee_func_free_t free_func);

ret_t cherokee_avl_add       (cherokee_avl_t *avl, cherokee_buffer_t *key, void  *value);
ret_t cherokee_avl_del       (cherokee_avl_t *avl, cherokee_buffer_t *key, void **value);
ret_t cherokee_avl_get       (cherokee_avl_t *avl, cherokee_buffer_t *key, void **value);

ret_t cherokee_avl_add_ptr   (cherokee_avl_t *avl, const char *key, void  *value);
ret_t cherokee_avl_del_ptr   (cherokee_avl_t *avl, const char *key, void **value);
ret_t cherokee_avl_get_ptr   (cherokee_avl_t *avl, const char *key, void **value);

ret_t cherokee_avl_len       (cherokee_avl_t *avl, size_t *len);
ret_t cherokee_avl_while     (cherokee_avl_t *avl, cherokee_avl_while_func_t func, void *param, cherokee_buffer_t **key, void **value);

ret_t cherokee_avl_set_case  (cherokee_avl_t *avl, cherokee_boolean_t case_sensitive);
ret_t cherokee_avl_check     (cherokee_avl_t *avl);
ret_t cherokee_avl_print     (cherokee_avl_t *avl);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_AVL_H */
