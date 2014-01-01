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

#include "common-internal.h"
#include "avl.h"


static ret_t
node_new (cherokee_avl_node_t **node, void *key)
{
	CHEROKEE_NEW_STRUCT (n, avl_node);

	cherokee_avl_generic_node_init (AVL_GENERIC_NODE(n));

	cherokee_buffer_init (&n->key);
	cherokee_buffer_add_buffer (&n->key, BUF(key));

	*node = n;
	return ret_ok;
}

static ret_t
node_mrproper (cherokee_avl_node_t *key)
{
	cherokee_buffer_mrproper (&key->key);
	return ret_ok;
}

static ret_t
node_free (cherokee_avl_node_t *key)
{
	ret_t ret;

	if (key == NULL)
		return ret_ok;

	ret = node_mrproper (key);
	free (key);
	return ret;
}

static int
node_cmp (cherokee_avl_node_t *A,
          cherokee_avl_node_t *B,
          cherokee_avl_t      *avl)
{
	if (AVL(avl)->case_insensitive) {
		return cherokee_buffer_case_cmp_buf (&A->key, &B->key);
	} else {
		return cherokee_buffer_cmp_buf (&A->key, &B->key);
	}
}

static int
node_is_empty (cherokee_avl_node_t *key)
{
	return cherokee_buffer_is_empty (&key->key);
}


ret_t
cherokee_avl_init (cherokee_avl_t *avl)
{
	cherokee_avl_generic_t *gen = AVL_GENERIC(avl);

	cherokee_avl_generic_init (gen);

	gen->node_mrproper = (avl_gen_node_mrproper_t) node_mrproper;
	gen->node_cmp      = (avl_gen_node_cmp_t) node_cmp;
	gen->node_is_empty = (avl_gen_node_is_empty_t) node_is_empty;

	avl->case_insensitive = false;

	return ret_ok;
}

ret_t
cherokee_avl_new (cherokee_avl_t **avl)
{
	CHEROKEE_NEW_STRUCT (n, avl);
	*avl = n;
	return cherokee_avl_init (n);
}


ret_t
cherokee_avl_add (cherokee_avl_t *avl, cherokee_buffer_t *key, void *value)
{
	ret_t                ret;
	cherokee_avl_node_t *n    = NULL;

	ret = node_new (&n, key);
	if ((ret != ret_ok) || (n == NULL)) {
		node_free (n);
		return ret;
	}

	return cherokee_avl_generic_add (AVL_GENERIC(avl), AVL_GENERIC_NODE(n), value);
}

ret_t
cherokee_avl_del (cherokee_avl_t *avl, cherokee_buffer_t *key, void **value)
{
	cherokee_avl_node_t tmp;

	cherokee_buffer_fake (&tmp.key, key->buf, key->len);

	return cherokee_avl_generic_del (AVL_GENERIC(avl), AVL_GENERIC_NODE(&tmp), value);
}

ret_t
cherokee_avl_get (cherokee_avl_t *avl, cherokee_buffer_t *key, void **value)
{
	cherokee_avl_node_t tmp;

	cherokee_buffer_fake (&tmp.key, key->buf, key->len);

	return cherokee_avl_generic_get (AVL_GENERIC(avl), AVL_GENERIC_NODE(&tmp), value);
}



ret_t
cherokee_avl_add_ptr (cherokee_avl_t *avl, const char *key, void *value)
{
	cherokee_buffer_t tmp_key;

	cherokee_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return cherokee_avl_add (avl, &tmp_key, value);
}

ret_t
cherokee_avl_del_ptr (cherokee_avl_t *avl, const char *key, void **value)
{
	cherokee_buffer_t tmp_key;

	cherokee_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return cherokee_avl_del (avl, &tmp_key, value);
}

ret_t
cherokee_avl_get_ptr (cherokee_avl_t *avl, const char *key, void **value)
{
	cherokee_buffer_t tmp_key;

	cherokee_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return cherokee_avl_get (avl, &tmp_key, value);
}



static ret_t
while_func_wrap (cherokee_avl_generic_node_t *node,
                 void                        *value,
                 void                        *params_internal)
{
	cherokee_buffer_t          *key;
	void                      **params    = (void **)params_internal;
	cherokee_avl_while_func_t   func_orig = params[0];
	void                       *param     = params[1];

	key = &AVL_NODE(node)->key;
	return func_orig (key, value, param);
}


ret_t
cherokee_avl_while (cherokee_avl_generic_t     *avl,
                    cherokee_avl_while_func_t   func,
                    void                       *param,
                    cherokee_buffer_t         **key,
                    void                      **value)
{
	void *params_internal[] = {func, param};

	return cherokee_avl_generic_while (avl, while_func_wrap, params_internal, (cherokee_avl_generic_node_t **) key, value);
}


ret_t
cherokee_avl_set_case (cherokee_avl_t     *avl,
                       cherokee_boolean_t  case_insensitive)
{
	avl->case_insensitive = case_insensitive;
	return ret_ok;
}
