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
#include "validator_file.h"
#include "connection-protected.h"

/* Properties
 */

ret_t
cherokee_validator_file_props_init_base (cherokee_validator_file_props_t *props,
                                         module_func_props_free_t         free_func)
{
	props->password_path_type = val_path_full;
	cherokee_buffer_init (&props->password_file);

	return cherokee_validator_props_init_base (VALIDATOR_PROPS(props), free_func);
}

ret_t
cherokee_validator_file_props_free_base (cherokee_validator_file_props_t *props)
{
	cherokee_buffer_mrproper (&props->password_file);

	return cherokee_validator_props_free_base (VALIDATOR_PROPS(props));
}

ret_t
cherokee_validator_file_configure (cherokee_config_node_t     *conf,
                                   cherokee_server_t          *srv,
                                   cherokee_module_props_t  **_props)
{
	ret_t                            ret;
	cherokee_config_node_t          *subconf;
	cherokee_validator_file_props_t *props    = PROP_VFILE(*_props);

	UNUSED (srv);

	/* Password file
	 */
	ret = cherokee_config_node_get (conf, "passwdfile", &subconf);
	if (ret == ret_ok) {
		cherokee_buffer_add_buffer (&props->password_file, &subconf->val);
	}

	/* Path type
	 */
	ret = cherokee_config_node_get (conf, "passwdfile_path", &subconf);
	if (ret == ret_ok) {
		if (equal_buf_str (&subconf->val, "full")) {
			props->password_path_type = val_path_full;
		} else if (equal_buf_str (&subconf->val, "local_dir")) {
			props->password_path_type = val_path_local_dir;
		} else {
			LOG_ERROR (CHEROKEE_ERROR_VALIDATOR_FILE, subconf->val.buf);
			return ret_error;
		}
	}

	/* Final checks
	 */
	if (cherokee_buffer_is_empty (&props->password_file)) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_VALIDATOR_FILE_NO_FILE);
		return ret_error;
	}

	return ret_ok;
}


/* Validator
 */

ret_t
cherokee_validator_file_init_base (cherokee_validator_file_t        *validator,
                                   cherokee_validator_file_props_t  *props,
                                   cherokee_plugin_info_validator_t *info)
{
	return cherokee_validator_init_base (VALIDATOR(validator),
	                                     VALIDATOR_PROPS(props), info);
}

ret_t
cherokee_validator_file_free_base (cherokee_validator_file_t *validator)
{
	return cherokee_validator_free_base (VALIDATOR(validator));
}


/* Utilities
 */

ret_t
cherokee_validator_file_get_full_path (cherokee_validator_file_t  *validator,
                                       cherokee_connection_t      *conn,
                                       cherokee_buffer_t         **ret_buf,
                                       cherokee_buffer_t          *tmp)
{
	cherokee_validator_file_props_t *props = VAL_VFILE_PROP(validator);

	switch (props->password_path_type) {
	case val_path_full:
		*ret_buf = &props->password_file;
		return ret_ok;

	case val_path_local_dir:
		cherokee_buffer_clean (tmp);
		cherokee_buffer_add_buffer (tmp, &conn->local_directory);
		cherokee_buffer_add_char   (tmp, '/');
		cherokee_buffer_add_buffer (tmp, &props->password_file);

		*ret_buf = tmp;
		return ret_ok;

	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}
