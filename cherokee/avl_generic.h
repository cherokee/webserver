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

#ifndef CHEROKEE_AVL_GENERIC_H
#define CHEROKEE_AVL_GENERIC_H

#include <cherokee/buffer.h>

CHEROKEE_BEGIN_DECLS


/* AVL Tree Node base
 */

typedef struct cherokee_avl_generic_node cherokee_avl_generic_node_t;

struct cherokee_avl_generic_node {
	short                             balance;
	struct cherokee_avl_generic_node *left;
	struct cherokee_avl_generic_node *right;
	cherokee_boolean_t                left_child;
	cherokee_boolean_t                right_child;
	void                             *value;
};

#define AVL_GENERIC_NODE(a) ((cherokee_avl_generic_node_t *)(a))

ret_t cherokee_avl_generic_node_init (cherokee_avl_generic_node_t *node);


/* AVL Tree
 */

typedef struct cherokee_avl_generic cherokee_avl_generic_t;

typedef ret_t (*avl_gen_node_mrproper_t) (cherokee_avl_generic_node_t *);
typedef ret_t (*avl_gen_node_cmp_t)      (cherokee_avl_generic_node_t *, cherokee_avl_generic_node_t *, cherokee_avl_generic_t *);
typedef ret_t (*avl_gen_node_is_empty_t) (cherokee_avl_generic_node_t *);


struct cherokee_avl_generic {
	cherokee_avl_generic_node_t *root;

	/* Virtual methods */
	avl_gen_node_mrproper_t node_mrproper;
	avl_gen_node_cmp_t      node_cmp;
	avl_gen_node_is_empty_t node_is_empty;
};

#define AVL_GENERIC(a) ((cherokee_avl_generic_t *)(a))

ret_t cherokee_avl_generic_init (cherokee_avl_generic_t *avl);


/* Public Methods
 */
typedef ret_t (* cherokee_avl_generic_while_func_t) (cherokee_avl_generic_node_t *key, void *value, void *param);

ret_t cherokee_avl_mrproper (cherokee_avl_generic_t *avl, cherokee_func_free_t free_func);
ret_t cherokee_avl_free     (cherokee_avl_generic_t *avl, cherokee_func_free_t free_func);
ret_t cherokee_avl_check    (cherokee_avl_generic_t *avl);
ret_t cherokee_avl_len      (cherokee_avl_generic_t *avl, size_t *len);
int   cherokee_avl_is_empty (cherokee_avl_generic_t *avl);

ret_t cherokee_avl_generic_add (cherokee_avl_generic_t *avl, cherokee_avl_generic_node_t *key, void  *value);
ret_t cherokee_avl_generic_del (cherokee_avl_generic_t *avl, cherokee_avl_generic_node_t *key, void **value);
ret_t cherokee_avl_generic_get (cherokee_avl_generic_t *avl, cherokee_avl_generic_node_t *key, void **value);

ret_t cherokee_avl_generic_while (cherokee_avl_generic_t             *avl,
                                  cherokee_avl_generic_while_func_t   func,
                                  void                               *param,
                                  cherokee_avl_generic_node_t       **key,
                                  void                              **value);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_AVL_GENERIC_H */
