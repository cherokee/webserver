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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_CONFIG_ENTRY_H
#define CHEROKEE_CONFIG_ENTRY_H

#include "avl.h"
#include "handler.h"
#include "http.h"
#include "validator.h"

#define CHEROKEE_CONFIG_PRIORITY_NONE    0
#define CHEROKEE_CONFIG_PRIORITY_DEFAULT 1

typedef enum {
	cherokee_expiration_none,
	cherokee_expiration_epoch,
	cherokee_expiration_max,
	cherokee_expiration_time
} cherokee_expiration_t;

typedef struct {
	/* Properties
	 */
	cherokee_buffer_t          *document_root;
	cherokee_boolean_t          only_secure;
	void                       *access;

	/* Handler
	 */
	handler_func_new_t          handler_new_func;
	cherokee_http_method_t      handler_methods;
	cherokee_module_props_t    *handler_properties; 

	/* Validator
	 */
	validator_func_new_t        validator_new_func;
	cherokee_module_props_t    *validator_properties; 

	cherokee_buffer_t          *auth_realm;
	cherokee_http_auth_t        authentication;
	cherokee_avl_t             *users;

	/* Headers
	 */
	cherokee_expiration_t       expiration;
	time_t                      expiration_time;

	/* Encoding
	 */
	cherokee_avl_t             *encoders;

} cherokee_config_entry_t; 

#define CONF_ENTRY(x) ((cherokee_config_entry_t *)(x))


ret_t cherokee_config_entry_new      (cherokee_config_entry_t **entry);
ret_t cherokee_config_entry_free     (cherokee_config_entry_t  *entry);
ret_t cherokee_config_entry_init     (cherokee_config_entry_t  *entry);
ret_t cherokee_config_entry_mrproper (cherokee_config_entry_t  *entry);

ret_t cherokee_config_entry_add_encoder (cherokee_config_entry_t *entry, cherokee_buffer_t *name, cherokee_plugin_info_t *plugin_info);
ret_t cherokee_config_entry_set_handler (cherokee_config_entry_t *entry, cherokee_plugin_info_handler_t *plugin_info); 
ret_t cherokee_config_entry_complete    (cherokee_config_entry_t *entry, cherokee_config_entry_t *source);

ret_t cherokee_config_entry_print       (cherokee_config_entry_t *entry);

#endif /* CHEROKEE_CONFIG_ENTRY_H */
