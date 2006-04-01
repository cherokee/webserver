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

#ifndef CHEROKEE_CONFIG_ENTRY_H
#define CHEROKEE_CONFIG_ENTRY_H

#include "common-internal.h"

#include "table.h"
#include "handler.h"
#include "encoder.h"
#include "module.h"
#include "validator.h"
#include "http.h"
#include "typed_table.h"


#define CHEROKEE_CONFIG_PRIORITY_NONE    0
#define CHEROKEE_CONFIG_PRIORITY_DEFAULT 1


typedef struct {
	/* Parent table_entry
	 */
	void                   *parent;
	cuint_t                 priority;

	/* Properties
	 */
	cherokee_buffer_t      *document_root;
	cherokee_boolean_t      only_secure;
	void                   *access;

	/* Handler
	 */
	handler_func_new_t      handler_new_func;
	cherokee_http_method_t  handler_methods;
	cherokee_table_t       *handler_properties; 

	/* Validator
	 */
	validator_func_new_t    validator_new_func;
	cherokee_table_t       *validator_properties; 

	cherokee_buffer_t      *auth_realm;
	cherokee_http_auth_t    authentication;
	cherokee_table_t       *users;

} cherokee_config_entry_t; 

#define CONF_ENTRY(x) ((cherokee_config_entry_t *)(x))


ret_t cherokee_config_entry_new  (cherokee_config_entry_t **entry);
ret_t cherokee_config_entry_free (cherokee_config_entry_t  *entry);
ret_t cherokee_config_entry_init (cherokee_config_entry_t  *entry);

ret_t cherokee_config_entry_set_handler_prop   (cherokee_config_entry_t *entry, char *prop_name, cherokee_typed_table_types_t type, void *value, cherokee_table_free_item_t free_func);
ret_t cherokee_config_entry_set_validator_prop (cherokee_config_entry_t *entry, char *prop_name, cherokee_typed_table_types_t type, void *value, cherokee_table_free_item_t free_func);
ret_t cherokee_config_entry_set_handler        (cherokee_config_entry_t *entry, cherokee_module_info_t *modinfo); 

ret_t cherokee_config_entry_complete    (cherokee_config_entry_t *entry, cherokee_config_entry_t *parent, cherokee_boolean_t same_type);
ret_t cherokee_config_entry_inherit     (cherokee_config_entry_t *entry);

#endif /* CHEROKEE_CONFIG_ENTRY_H */
