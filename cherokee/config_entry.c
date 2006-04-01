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
#include "config_entry.h"

#include "access.h"
#include "http.h"

typedef enum {
	table_handler,
	table_validator
} prop_table_types_t;


ret_t 
cherokee_config_entry_new (cherokee_config_entry_t **entry)
{
	CHEROKEE_NEW_STRUCT (n, config_entry);
		
	cherokee_config_entry_init (n);

	*entry = n;	
	return ret_ok;
}


ret_t 
cherokee_config_entry_init (cherokee_config_entry_t *entry)
{
	entry->parent               = NULL;
	entry->priority             = CHEROKEE_CONFIG_PRIORITY_NONE;

	entry->handler_new_func     = NULL;
	entry->handler_properties   = NULL;
	entry->handler_methods      = http_unknown;

	entry->validator_new_func   = NULL;
	entry->validator_properties = NULL;

	entry->access               = NULL;
	entry->authentication       = http_auth_nothing;
	entry->only_secure          = false;

	entry->document_root        = NULL;
	entry->auth_realm           = NULL;
	entry->users                = NULL;

	return ret_ok;
}


ret_t 
cherokee_config_entry_free (cherokee_config_entry_t *entry) 
{
	if (entry->handler_properties != NULL) {
		cherokee_typed_table_free (entry->handler_properties);
		entry->handler_properties = NULL;
	}

	if (entry->validator_properties != NULL) {
		cherokee_typed_table_free (entry->validator_properties);
		entry->validator_properties = NULL;
	}
	
	if (entry->access != NULL) {
		cherokee_access_free (entry->access);
		entry->access = NULL;
	}

	if (entry->document_root != NULL) {
		cherokee_buffer_free (entry->document_root);
		entry->document_root = NULL;
	}

	if (entry->auth_realm != NULL) {
		cherokee_buffer_free (entry->auth_realm);
		entry->auth_realm = NULL;
	}

	if (entry->users != NULL) {
		cherokee_table_free (entry->users);
		entry->users = NULL;
	}

	free (entry);
	return ret_ok;
}


static ret_t 
entry_set_prop (prop_table_types_t table_type, cherokee_config_entry_t *entry, char *prop_name, cherokee_typed_table_types_t type, void *value, cherokee_table_free_item_t free_func)
{
	ret_t              ret;
	cherokee_table_t **table;

	/* Choose the table
	 */
	switch (table_type) {
	case table_handler:
		table = &entry->handler_properties;
		break;
	case table_validator:
		table = &entry->validator_properties;
		break;
	}
	
	/* Create the table on demand to save memory
	 */
	if (*table == NULL) {
		ret = cherokee_table_new (table);
		if (unlikely(ret != ret_ok)) return ret;
	}

	/* Add the property
	 */
	switch (type) {
	case typed_int:
		return cherokee_typed_table_add_int (*table, prop_name, POINTER_TO_INT(value));
	case typed_str:
		return cherokee_typed_table_add_str (*table, prop_name, value);
	case typed_data:
		return cherokee_typed_table_add_data (*table, prop_name, value, free_func);
	case typed_list:
		return cherokee_typed_table_add_list (*table, prop_name, value, free_func);
	default:
		SHOULDNT_HAPPEN;
	}
	
	return ret_error;
}


ret_t 
cherokee_config_entry_set_handler_prop (cherokee_config_entry_t *entry, char *prop_name, cherokee_typed_table_types_t type, void *value, cherokee_table_free_item_t free_func)
{
	return entry_set_prop (table_handler, entry, prop_name, type, value, free_func);
}


ret_t 
cherokee_config_entry_set_validator_prop (cherokee_config_entry_t *entry, char *prop_name, cherokee_typed_table_types_t type, void *value, cherokee_table_free_item_t free_func)
{
	return entry_set_prop (table_validator, entry, prop_name, type, value, free_func);
}


ret_t 
cherokee_config_entry_set_handler (cherokee_config_entry_t *entry, cherokee_module_info_t *modinfo)
{
	return_if_fail (modinfo != NULL, ret_error);

	if (modinfo->type != cherokee_handler) {
		PRINT_ERROR ("Directory '%s' has not a handler module!\n", entry->document_root->buf);
		return ret_error;
	}

	entry->handler_new_func = modinfo->new_func;
	entry->handler_methods  = MODULE_INFO_HANDLER(modinfo)->valid_methods;

	return ret_ok;
}


ret_t 
cherokee_config_entry_complete (cherokee_config_entry_t *entry, cherokee_config_entry_t *main, cherokee_boolean_t same_type)
{
	cherokee_boolean_t modified  = false;
	cherokee_boolean_t overwrite = false;

#define should_assign(t,s,prop,nil)  \
       	((t->prop == nil) || ((t->prop != nil) && (s->prop != nil) && overwrite))

	if (!same_type && (entry->priority < main->priority)) {
		overwrite = true;
	}

//	printf ("same_type=%d, overwrite=%d, prio=%d\n", same_type, overwrite, main->priority);

	/* If a temporary config_entry inherits from valid entry, it will
	 * get references than mustn't be free'd, like 'user'. Take care.
	 */
	if (entry->parent == NULL)
		entry->parent = main->parent;

	if (should_assign (entry, main, handler_properties, NULL))
		entry->handler_properties = main->handler_properties;

	if (should_assign (entry, main, validator_properties, NULL))
		entry->validator_properties = main->validator_properties;

	if (should_assign (entry, main, handler_new_func, NULL)) {
		entry->handler_new_func = main->handler_new_func;
		entry->handler_methods  = main->handler_methods;
		modified = true;
	}
	
	if (should_assign (entry, main, authentication, 0))
		entry->authentication = main->authentication;
	
	if (should_assign (entry, main, only_secure, false))
		entry->only_secure = main->only_secure;

	if (should_assign (entry, main, access, NULL))
		entry->access = main->access;
	
	if (should_assign (entry, main, validator_new_func, NULL))
		entry->validator_new_func = main->validator_new_func;

  	if (should_assign (entry, main, document_root, NULL))
 		entry->document_root = main->document_root;	 
	
 	if (should_assign (entry, main, auth_realm, NULL))
 		entry->auth_realm = main->auth_realm; 

	if (should_assign (entry, main, users, NULL))
		entry->users = main->users;

	/* Finally, assign the new priority to the entry
	 */
	if (entry->priority < main->priority) {
		entry->priority = main->priority;
	}

	return (modified) ? ret_ok : ret_eagain;
}


ret_t 
cherokee_config_entry_inherit (cherokee_config_entry_t *entry)
{
	cherokee_config_entry_t *parent = entry;

	while ((parent = parent->parent) != NULL) {
		cherokee_config_entry_complete (entry, parent, true);
	}

	return ret_ok;
}
