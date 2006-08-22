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
#include "module_loader.h"
#include "validator_ldap.h"


ret_t 
cherokee_validator_ldap_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_table_t **props)
{
	return ret_ok;
}

ret_t 
cherokee_validator_ldap_new (cherokee_validator_ldap_t **ldap, cherokee_validator_props_t *props)
{	  
	CHEROKEE_NEW_STRUCT(n,validator_ldap);

	/* Init 		
	 */
	cherokee_validator_init_base (VALIDATOR(n), props);
	VALIDATOR(n)->support = http_auth_basic | http_auth_digest;

	MODULE(n)->free           = (module_func_free_t)           cherokee_validator_ldap_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       cherokee_validator_ldap_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) cherokee_validator_ldap_add_headers;

	/* Get the properties
	 */

	/* ldap_init */

	*ldap = n;
	return ret_ok;
}

ret_t 
cherokee_validator_ldap_free (cherokee_validator_ldap_t *ldap)
{
	return ret_ok;
}

ret_t 
cherokee_validator_ldap_check (cherokee_validator_ldap_t *ldap, cherokee_connection_t *conn)
{
	return ret_ok;
}

ret_t 
cherokee_validator_ldap_add_headers (cherokee_validator_ldap_t *ldap, cherokee_connection_t *conn, cherokee_buffer_t *buf)
{
	return ret_ok;
}


/* Library init function
 */
void
MODULE_INIT(ldap) (cherokee_module_loader_t *loader)
{
}

VALIDATOR_MODULE_INFO_INIT_EASY (ldap, http_auth_basic);

