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

#ifndef CHEROKEE_AVL_R_H
#define CHEROKEE_AVL_R_H

#include <cherokee/avl.h>


CHEROKEE_BEGIN_DECLS

typedef struct {
	cherokee_avl_t  avl;
	void           *priv;
} cherokee_avl_r_t;

#define AVL_R(a) ((cherokee_avl_t *)(a))

ret_t cherokee_avl_r_init      (cherokee_avl_r_t  *avl_r);
ret_t cherokee_avl_r_mrproper  (cherokee_avl_r_t  *avl_r, cherokee_func_free_t free_func);

ret_t cherokee_avl_r_add       (cherokee_avl_r_t *avl_r, cherokee_buffer_t *key, void  *value);
ret_t cherokee_avl_r_del       (cherokee_avl_r_t *avl_r, cherokee_buffer_t *key, void **value);
ret_t cherokee_avl_r_get       (cherokee_avl_r_t *avl_r, cherokee_buffer_t *key, void **value);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_AVL_R_H */
