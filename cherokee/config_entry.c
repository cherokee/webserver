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
#include "config_entry.h"

#include "access.h"
#include "http.h"

typedef enum {
	table_handler,
	table_validator
} prop_table_types_t;


/* Implements _new() and _free() 
 */
CHEROKEE_ADD_FUNC_NEW  (config_entry);
CHEROKEE_ADD_FUNC_FREE (config_entry);


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
	entry->auth_realm           = NULL;

	entry->access               = NULL;
	entry->authentication       = http_auth_nothing;
	entry->only_secure          = false;

	entry->document_root        = NULL;
	entry->users                = NULL;

	return ret_ok;
}


ret_t 
cherokee_config_entry_mrproper (cherokee_config_entry_t *entry) 
{
	if (entry->handler_properties != NULL) {
		cherokee_module_props_free (entry->handler_properties);
		entry->handler_properties = NULL;
	}

	if (entry->validator_properties != NULL) {
		cherokee_module_props_free (entry->validator_properties);
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
		entry->document_root = NULL;
	}

	if (entry->users != NULL) {
		cherokee_avl_free (entry->users, free);
		entry->users = NULL;
	}

	return ret_ok;
}


ret_t 
cherokee_config_entry_set_handler (cherokee_config_entry_t *entry, cherokee_plugin_info_handler_t *plugin_info)
{
	return_if_fail (plugin_info != NULL, ret_error);

	if (PLUGIN_INFO(plugin_info)->type != cherokee_handler) {
		PRINT_ERROR ("Directory '%s' has not a handler module!\n", entry->document_root->buf);
		return ret_error;
	}

	entry->handler_new_func = PLUGIN_INFO(plugin_info)->instance;
	entry->handler_methods  = plugin_info->valid_methods;

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

/* 	printf ("same_type=%d, overwrite=%d, prio=%d\n", same_type, overwrite, main->priority); */

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


ret_t 
cherokee_config_entry_print (cherokee_config_entry_t *entry)
{
	printf ("parent:                    %p\n", entry->parent);
	printf ("priority:                  %d\n", entry->priority);
	printf ("document_root:             %s\n", entry->document_root ? entry->document_root->buf : "");
	printf ("only_secure:               %d\n", entry->only_secure);
	printf ("access:                    %p\n", entry->access);
	printf ("handler_new                %p\n", entry->handler_new_func);
	printf ("http_methods:              0x%x\n", entry->handler_methods);
	printf ("handler_properties:        %p\n", entry->handler_properties);
	printf ("validator_new:             %p\n", entry->validator_new_func);
	printf ("validator_properties:      %p\n", entry->validator_properties);
	printf ("auth_realm:                %s\n", entry->auth_realm ? entry->auth_realm->buf : "");
	printf ("users:                     %p\n", entry->users);

	return ret_ok;
}
