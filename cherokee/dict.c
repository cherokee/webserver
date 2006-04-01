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

#include "common-internal.h"
#include "dict.h"

#include "dict/dict_generic.h"
#include "dict/sp_tree.h"


#include <stdlib.h>
#include <string.h>


ret_t 
cherokee_dict_new (cherokee_dict_t        **dict, 
		   cherokee_dict_cmp_func_t key_cmp,
		   cherokee_dict_del_func_t key_del,
		   cherokee_dict_del_func_t val_del)
{
	cherokee_dict_t *n;
	dict_cmp_func    k_cmp;
        dict_del_func    k_del;
        dict_del_func    v_del;
	   
	k_cmp = (key_cmp) ? key_cmp : (dict_cmp_func)strcmp;
	k_del = (key_del) ? key_del : (dict_del_func)free;
	v_del = (val_del) ? val_del : (dict_del_func)free;

	n = sp_dict_new (k_cmp, k_del, v_del);
	if (unlikely(n == NULL)) return ret_error;

	*dict = n;
	return ret_ok;
}


ret_t 
cherokee_dict_free (cherokee_dict_t    *dict,
		    cherokee_boolean_t  free)
{
	dict_destroy (dict, free);
	return ret_ok;
}


ret_t 
cherokee_dict_clean (cherokee_dict_t    *dict, 
		     cherokee_boolean_t  free)
{
	dict_empty (dict, free);
	return ret_ok;
}


ret_t 
cherokee_dict_get (cherokee_dict_t *dict, char *key, void **value)
{
	void *val;
	   
	val = dict_search (dict, key);
	if (val == NULL) return ret_not_found;

	*value = val;
	return ret_ok;
}


ret_t 
cherokee_dict_add (cherokee_dict_t *dict, void *key, void  *value, cherokee_boolean_t overwrite)
{
	int re;

	re = dict_insert (dict, key, value, overwrite);
	switch (re) {
	case  0: 
		return ret_ok;    /* ok, inserted */
	case  1: 
		return ret_ok;    /* ok, not inserted because overwrite was false */
	case -1: 
		return ret_error; /* Opsss */
	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}


ret_t 
cherokee_dict_len (cherokee_dict_t *dict, unsigned int *len)
{
	int l;

	l = dict_count (dict);

	*len = l;
	return ret_ok;
}


ret_t 
cherokee_dict_while (cherokee_dict_t            *dict, 
		     cherokee_dict_while_func_t  func,
		     void                       *param,
		     void                      **key,
		     void                      **value)
{
	int found;
	   
	found = sp_tree_walk2 (dict->_object, func, param, key, value);
	if (!found) return ret_not_found;
	   
	return ret_ok;
}
