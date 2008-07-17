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
#include "avl_r.h"

typedef struct {
	CHEROKEE_RWLOCK_T(lock);
	int dummy;
} cherokee_avl_r_priv_t;

#define AVL_R_PRIV(avl_r) ((cherokee_avl_r_priv_t *)((avl_r)->priv))
#define AVL_R_LOCK(avl_r) (&AVL_R_PRIV(avl_r)->lock)


ret_t
cherokee_avl_r_init (cherokee_avl_r_t *avl_r)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n, avl_r_priv);

	ret = cherokee_avl_init (&avl_r->avl);
	if (ret != ret_ok) 
		return ret;

	avl_r->priv = n;
	CHEROKEE_RWLOCK_INIT (AVL_R_LOCK(avl_r), NULL);

	return ret_ok;
}


ret_t 
cherokee_avl_r_mrproper (cherokee_avl_r_t *avl_r, cherokee_func_free_t free_func)
{
	if (avl_r->priv) {
		CHEROKEE_RWLOCK_DESTROY (AVL_R_LOCK(avl_r));
		free (avl_r->priv);
	}

	return cherokee_avl_mrproper (&avl_r->avl, free_func);
}


ret_t
cherokee_avl_r_add (cherokee_avl_r_t *avl_r, cherokee_buffer_t *key, void *value)
{
	ret_t ret;

	CHEROKEE_RWLOCK_WRITER (AVL_R_LOCK(avl_r));
	ret = cherokee_avl_add (&avl_r->avl, key, value);
	CHEROKEE_RWLOCK_UNLOCK (AVL_R_LOCK(avl_r));

	return ret;
}


ret_t 
cherokee_avl_r_del (cherokee_avl_r_t *avl_r, cherokee_buffer_t *key, void **value)
{
	ret_t ret;

	CHEROKEE_RWLOCK_WRITER (AVL_R_LOCK(avl_r));
	ret = cherokee_avl_del (&avl_r->avl, key, value);
	CHEROKEE_RWLOCK_UNLOCK (AVL_R_LOCK(avl_r));

	return ret;
}


ret_t 
cherokee_avl_r_get (cherokee_avl_r_t *avl_r, cherokee_buffer_t *key, void **value)
{
	ret_t ret;

	CHEROKEE_RWLOCK_READER (AVL_R_LOCK(avl_r));
	ret = cherokee_avl_get (&avl_r->avl, key, value);
	CHEROKEE_RWLOCK_UNLOCK (AVL_R_LOCK(avl_r));

	return ret;
}
