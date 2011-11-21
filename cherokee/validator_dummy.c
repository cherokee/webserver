/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
#include "validator_dummy.h"
#include "connection-protected.h"

/* Plug-in initialization
 */
PLUGIN_INFO_VALIDATOR_EASIEST_INIT (dummy, http_auth_basic);

ret_t
cherokee_validator_dummy_new (cherokee_validator_dummy_t **dummy, cherokee_module_props_t *props)
{
	
	CHEROKEE_NEW_STRUCT (n, validator_dummy);

	cherokee_validator_init_base (VALIDATOR(n), VALIDATOR_PROPS(props), PLUGIN_INFO_VALIDATOR_PTR(dummy));
	VALIDATOR(n)->support = http_auth_basic;

	VALIDATOR(n)->check   = (validator_func_check_t) cherokee_validator_dummy_check;


	/* Return obj
	 */
	*dummy = n;
	return ret_ok;
}

ret_t
cherokee_validator_dummy_configure (cherokee_config_node_t    *conf,
				   cherokee_server_t          *srv,
				   cherokee_module_props_t  **_props)
{
	UNUSED(conf);
	UNUSED(srv);

	if(*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, validator_dummy_props);

		cherokee_validator_props_init_base (VALIDATOR_PROPS (n), MODULE_PROPS_FREE(cherokee_validator_props_free_base));

		*_props = MODULE_PROPS (n);
	}

	return ret_ok;
}

ret_t
cherokee_validator_dummy_check (cherokee_validator_dummy_t *dummy, cherokee_connection_t *conn)
{
	UNUSED(dummy);

	if ((conn->validator->user.len > 0) || (conn->validator->passwd.len > 0))
		return ret_ok;
	
	return ret_deny;
}
