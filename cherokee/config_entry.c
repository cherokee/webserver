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
#include "util.h"
#include "encoder.h"

#define ENTRIES "config,rules"


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

	entry->expiration           = cherokee_expiration_none;
	entry->expiration_time      = 0;

	entry->encoders             = NULL;

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
	
	if (entry->encoders) {
		cherokee_avl_mrproper (entry->encoders, NULL);
		entry->encoders = NULL;
	}

	return ret_ok;
}

ret_t
cherokee_config_entry_add_encoder (cherokee_config_entry_t *entry, 
				   cherokee_buffer_t       *name,
				   cherokee_plugin_info_t  *plugin_info)
{
	if (entry->encoders == NULL) {
		cherokee_avl_new (&entry->encoders);
	}

	cherokee_avl_add (entry->encoders, name, (void*)plugin_info->instance);
	return ret_ok;
}


ret_t 
cherokee_config_entry_set_handler (cherokee_config_entry_t        *entry,
				   cherokee_plugin_info_handler_t *plugin_info)
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
cherokee_config_entry_complete (cherokee_config_entry_t *entry, cherokee_config_entry_t *source)
{
	/* This method is assigning pointer to the server data. The
	 * target entry properties must NOT be freed. Take care.
	 */

	if (! entry->handler_properties)
		entry->handler_properties = source->handler_properties;

	if (! entry->validator_properties)
		entry->validator_properties = source->validator_properties;

	if (! entry->handler_new_func) {
		entry->handler_new_func = source->handler_new_func;
		entry->handler_methods  = source->handler_methods;
	}

	if (entry->authentication == 0)
		entry->authentication = source->authentication;
	
	if (entry->only_secure == false)
		entry->only_secure = source->only_secure;

	if (! entry->access)
		entry->access = source->access;
	
	if (! entry->validator_new_func)
		entry->validator_new_func = source->validator_new_func;

	if (! entry->document_root)
 		entry->document_root = source->document_root;	 
	
	if (! entry->auth_realm)
 		entry->auth_realm = source->auth_realm; 

	if (! entry->users)
		entry->users = source->users;
	
	if ((entry->expiration  == cherokee_expiration_none) &&
	    (source->expiration != cherokee_expiration_none))
	{
		entry->expiration      = source->expiration;
		entry->expiration_time = source->expiration_time;
	}

	if (! entry->encoders)
		entry->encoders = source->encoders;

	return ret_ok;
}


ret_t 
cherokee_config_entry_print (cherokee_config_entry_t *entry)
{
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
	printf ("expiration type:           %d\n", entry->expiration);
	printf ("expiration_time            %lu\n", entry->expiration_time);
	printf ("encoders_accepted          %p\n", entry->encoders);

	return ret_ok;
}
