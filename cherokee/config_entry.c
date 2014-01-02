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
#include "config_entry.h"

#include "access.h"
#include "http.h"
#include "util.h"
#include "encoder.h"
#include "header_op.h"

#define ENTRIES "config,rules"


typedef enum {
	table_handler,
	table_validator
} prop_table_types_t;


/* Implements _new() and _free()
 */
CHEROKEE_ADD_FUNC_NEW  (config_entry);
CHEROKEE_ADD_FUNC_FREE (config_entry);


static ret_t
init (struct cherokee_config_entry *entry)
{
	entry->handler_new_func          = NULL;
	entry->handler_properties        = NULL;
	entry->handler_methods           = http_unknown;

	entry->validator_new_func        = NULL;
	entry->validator_properties      = NULL;
	entry->auth_realm                = NULL;

	entry->access                    = NULL;
	entry->authentication            = http_auth_nothing;
	entry->only_secure               = false;
	entry->header_ops                = NULL;

	entry->document_root             = NULL;
	entry->users                     = NULL;

	entry->expiration                = cherokee_expiration_none;
	entry->expiration_time           = 0;
	entry->expiration_prop           = cherokee_expiration_prop_none;

	entry->flcache                   = NULLB_NULL;
	entry->flcache_policy            = flcache_policy_explicitly_allowed;
	entry->flcache_cookies_disregard = NULL;

	entry->encoders                  = NULL;
	entry->limit_bps                 = 0;
	entry->no_log                    = NULLB_NULL;

	entry->timeout_lapse             = NULLI_NULL;
	entry->timeout_header            = NULL;

	return ret_ok;
}


ret_t
cherokee_config_entry_init (cherokee_config_entry_t *entry)
{
	return init (entry);
}


ret_t
cherokee_config_entry_mrproper (cherokee_config_entry_t *entry)
{
	cherokee_list_t *i, *tmp;

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
		entry->auth_realm = NULL;
	}

	if (entry->users != NULL) {
		cherokee_avl_free (AVL_GENERIC(entry->users), free);
		entry->users = NULL;
	}

	if (entry->encoders) {
		cherokee_avl_free (AVL_GENERIC(entry->encoders), (cherokee_func_free_t) cherokee_module_props_free);
		entry->encoders = NULL;
	}

	if (entry->header_ops != NULL) {
		list_for_each_safe (i, tmp, entry->header_ops) {
			cherokee_list_del (i);
			cherokee_header_op_free (HEADER_OP(i));
		}

		free (entry->header_ops);
		entry->header_ops = NULL;
	}

	if (entry->flcache_cookies_disregard != NULL) {
		list_for_each_safe (i, tmp, entry->flcache_cookies_disregard) {
			cherokee_list_del (i);
			free (i);
		}

		free (entry->flcache_cookies_disregard);
		entry->flcache_cookies_disregard = NULL;
	}

	if (entry->timeout_header != NULL) {
		cherokee_buffer_free (entry->timeout_header);
		entry->timeout_header = NULL;
	}

	return ret_ok;
}


ret_t
cherokee_config_entry_set_encoder (cherokee_config_entry_t  *entry,
                                   cherokee_buffer_t        *encoder_name,
                                   cherokee_plugin_info_t   *plugin_info,
                                   cherokee_encoder_props_t *encoder_props)
{
	/* Sanity checks
	 */
	return_if_fail (plugin_info != NULL, ret_error);

	if (unlikely (PLUGIN_INFO(plugin_info)->type != cherokee_encoder)) {
		LOG_ERROR_S (CHEROKEE_ERROR_CONFIG_ENTRY_BAD_TYPE);
		return ret_error;
	}

	/* Add the encoder property obj to the tree
	 */
	if (entry->encoders == NULL) {
		cherokee_avl_new (&entry->encoders);
	}

	return cherokee_avl_add (entry->encoders, encoder_name, (void*)encoder_props);
}


ret_t
cherokee_config_entry_set_handler (cherokee_config_entry_t        *entry,
                                   cherokee_plugin_info_handler_t *plugin_info)
{
	return_if_fail (plugin_info != NULL, ret_error);

	if (unlikely (PLUGIN_INFO(plugin_info)->type != cherokee_handler)) {
		LOG_ERROR_S (CHEROKEE_ERROR_CONFIG_ENTRY_BAD_TYPE);
		return ret_error;
	}

	entry->handler_new_func = PLUGIN_INFO(plugin_info)->instance;
	entry->handler_methods  = plugin_info->valid_methods;

	return ret_ok;
}


ret_t
cherokee_config_entry_complete (cherokee_config_entry_t *entry,
                                cherokee_config_entry_t *source)
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
		entry->expiration_prop = source->expiration_prop;
	}

	if (! entry->encoders)
		entry->encoders = source->encoders;

	if (! entry->limit_bps)
		entry->limit_bps = source->limit_bps;

	if (NULLB_IS_NULL (entry->no_log)) {
		entry->no_log = source->no_log;
	}

	if (NULLB_IS_NULL (entry->flcache)) {
		entry->flcache        = source->flcache;
		entry->flcache_policy = source->flcache_policy;
	}

	if ((entry->flcache) &&
	    (! entry->flcache_cookies_disregard) &&
	    ( source->flcache_cookies_disregard))
	{
		entry->flcache_cookies_disregard = source->flcache_cookies_disregard;
	}

	if (NULLI_IS_NULL(entry->timeout_lapse) && (source->timeout_lapse != NULLI_NULL))
	{
		entry->timeout_lapse  = source->timeout_lapse;
		entry->timeout_header = source->timeout_header;
	}

	if ((! entry->header_ops) && (source->header_ops)) {
		entry->header_ops = source->header_ops;
	}

	return ret_ok;
}


ret_t
cherokee_config_entry_print (cherokee_config_entry_t *entry)
{
	printf ("document_root:             %s\n", entry->document_root ? entry->document_root->buf : "");
	printf ("only_secure:               %d\n", entry->only_secure);
	printf ("access:                    %p\n", entry->access);
	printf ("handler_new:               %p\n", entry->handler_new_func);
	printf ("http_methods:              0x%x\n", (cuint_t)entry->handler_methods);
	printf ("handler_properties:        %p\n", entry->handler_properties);
	printf ("validator_new:             %p\n", entry->validator_new_func);
	printf ("validator_properties:      %p\n", entry->validator_properties);
	printf ("auth_realm:                %s\n", entry->auth_realm ? entry->auth_realm->buf : "");
	printf ("users:                     %p\n", entry->users);
	printf ("expiration type:           %d\n", entry->expiration);
	printf ("expiration time:           %lu\n", entry->expiration_time);
	printf ("flcache:                   %s\n", NULLB_TO_STR(entry->flcache));
	printf ("flcache policy:            %d\n", entry->flcache_policy);
	printf ("encoders_accepted:         %p\n", entry->encoders);
	printf ("limit bps:                 %d\n", entry->limit_bps);
	printf ("no_log:                    %s\n", NULLB_TO_STR(entry->no_log));

	if (NULLI_IS_NULL(entry->timeout_lapse)) {
		printf ("timeout custom:                  no\n");
	} else {
		printf ("timeout custom:                  %d\n", entry->timeout_lapse);
	}

	return ret_ok;
}


/* Entry reference
 */

ret_t
cherokee_config_entry_ref_init (cherokee_config_entry_ref_t *entry)
{
	return init (entry);
}

ret_t
cherokee_config_entry_ref_clean (cherokee_config_entry_ref_t *entry)
{
	return init (entry);
}
