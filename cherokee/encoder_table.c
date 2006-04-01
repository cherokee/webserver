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

#include "encoder_table.h"


ret_t 
cherokee_encoder_table_new  (cherokee_encoder_table_t **et)
{
	CHEROKEE_NEW_STRUCT(n, encoder_table);

	/* Init the table
	 */
	cherokee_table_init (&n->table);

	/* Return the object
	 */
	*et = n;	
	return ret_ok;
}


static void
for_each_func_free_encoder (const char *key, void *encoder)
{	
	cherokee_encoder_free (encoder);
}


ret_t 
cherokee_encoder_table_free (cherokee_encoder_table_t *et)
{
	cherokee_table_foreach (&et->table, for_each_func_free_encoder);
	
	/* alo: It's ok, have a look at cherokee_encoder_table_t before change it
	 */
	return cherokee_table_free (&et->table);
}


ret_t 
cherokee_encoder_table_set (cherokee_encoder_table_t *et, char *encoder, cherokee_encoder_table_entry_t *entry)
{
	return cherokee_table_add (&et->table, encoder, entry);
}

ret_t
cherokee_encoder_table_get (cherokee_encoder_table_t *et, char *encoder, cherokee_encoder_table_entry_t **entry) 
{
	return cherokee_table_get (&et->table, encoder, (void **)entry);
}


ret_t 
cherokee_encoder_table_new_encoder (cherokee_encoder_table_t *et, char *encoder, char *ext, cherokee_encoder_t **new_encoder)
{
	ret_t ret;
	int   make_object = 1;
	cherokee_encoder_table_entry_t *entry;
	cherokee_matching_list_t       *matching;

	ret = cherokee_encoder_table_get (et, encoder, &entry);
	if (unlikely(ret != ret_ok)) return ret;

	if (cherokee_encoder_entry_has_matching_list (entry)) {
		ret = cherokee_encoder_entry_get_matching_list (entry, &matching);
		if (unlikely(ret != ret_ok)) return ret;

		make_object = cherokee_matching_list_match (matching, ext);
	}

	if (make_object) {
		encoder_func_new_t new_func;

		new_func = entry->func_new;
		ret = new_func ((void **)new_encoder);
		if (unlikely(ret != ret_ok)) return ret;
	}

	return ret_ok;
}


/* 
 * Encoder entry methods 
 */

ret_t 
cherokee_encoder_table_entry_new (cherokee_encoder_table_entry_t **eentry)
{
	CHEROKEE_NEW_STRUCT (n, encoder_table_entry);

	n->matching = NULL;
	n->func_new = NULL;

	*eentry = n;

	return ret_ok;
}


ret_t 
cherokee_encoder_table_entry_get_info  (cherokee_encoder_table_entry_t *eentry, cherokee_module_info_t *info)
{
	if (info->type != cherokee_encoder) {
		PRINT_ERROR ("Wrong module: type(%d) is not a cherokee_encoder\n", info->type);
		return ret_error;
	}

	eentry->func_new = info->new_func;

	return ret_ok;
}



ret_t 
cherokee_encoder_entry_set_matching_list (cherokee_encoder_table_entry_t *eentry, cherokee_matching_list_t *matching)
{
	eentry->matching = matching;
	return ret_ok;
}

int
cherokee_encoder_entry_has_matching_list (cherokee_encoder_table_entry_t *eentry)
{
	return (eentry->matching != NULL);
}


ret_t 
cherokee_encoder_entry_get_matching_list (cherokee_encoder_table_entry_t *eentry, cherokee_matching_list_t **matching)
{
	*matching = eentry->matching;
	return ret_ok;
}
