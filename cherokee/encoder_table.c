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

#include "encoder_table.h"
#include "util.h"

#define ENTRIES "encoder"


ret_t 
cherokee_encoder_table_init  (cherokee_encoder_table_t *et)
{
	return cherokee_avl_init (et);
}


ret_t 
cherokee_encoder_table_mrproper (cherokee_encoder_table_t *et)
{
	return cherokee_avl_mrproper (et, (cherokee_func_free_t)cherokee_encoder_table_entry_free);
}


ret_t 
cherokee_encoder_table_set (cherokee_encoder_table_t *et, cherokee_buffer_t *encoder, cherokee_encoder_table_entry_t *entry)
{
	return cherokee_avl_add (et, encoder, entry);
}


ret_t
cherokee_encoder_table_get (cherokee_encoder_table_t *et, char *encoder, cherokee_encoder_table_entry_t **entry) 
{
	return cherokee_avl_get_ptr (et, encoder, (void **)entry);
}


ret_t 
cherokee_encoder_table_new_encoder (cherokee_encoder_table_t *et, char *encoder, char *ext, cherokee_encoder_t **new_encoder)
{
	ret_t                           ret;
	cherokee_encoder_table_entry_t *entry;
	cherokee_matching_list_t       *matching;
	cherokee_boolean_t              make_object = true;

	ret = cherokee_encoder_table_get (et, encoder, &entry);
	if (unlikely(ret != ret_ok)) {
		TRACE (ENTRIES, "Encoder table: '%s' not found\n", encoder);
		return ret;
	}

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


/* Encoder properties
 */
ret_t 
cherokee_encoder_table_entry_new (cherokee_encoder_table_entry_t **eentry)
{
	CHEROKEE_NEW_STRUCT (n, encoder_table_entry);

	n->func_new = NULL;
	n->matching = NULL;

	*eentry = n;
	return ret_ok;
}

ret_t 
cherokee_encoder_table_entry_free (cherokee_encoder_table_entry_t *eentry)
{
	if (eentry->matching != NULL) {
		cherokee_matching_list_free (eentry->matching);
	}

	free (eentry);
	return ret_ok;
}


ret_t 
cherokee_encoder_table_entry_get_info  (cherokee_encoder_table_entry_t *eentry, cherokee_plugin_info_t *info)
{
	if (info->type != cherokee_encoder) {
		PRINT_ERROR ("ERROR: Wrong module type(%d): not a encoder\n", info->type);
		return ret_error;
	}

	eentry->func_new = info->instance;
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
