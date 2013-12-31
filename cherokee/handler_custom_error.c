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
#include "handler_custom_error.h"
#include "connection-protected.h"
#include "util.h"

PLUGIN_INFO_HANDLER_EASIEST_INIT (custom_error, http_all_methods);


static ret_t
props_free (cherokee_handler_custom_error_props_t *props)
{
	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


ret_t
cherokee_handler_custom_error_configure (cherokee_config_node_t   *conf,
                                         cherokee_server_t        *srv,
                                         cherokee_module_props_t **_props)
{
	ret_t                                  ret;
	int                                    val;
	cherokee_list_t                       *i;
	cherokee_config_node_t                *subconf;
	cherokee_handler_custom_error_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_custom_error_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
		                                  MODULE_PROPS_FREE(props_free));

		n->error_code = http_unset;
		*_props = MODULE_PROPS(n);
	}

	props = PROP_CUSTOM_ERROR(*_props);

	cherokee_config_node_foreach (i, conf) {
		subconf = CONFIG_NODE(i);
		if (equal_buf_str (&subconf->key, "error")) {
			ret = cherokee_atoi (subconf->val.buf, &val);
			if (ret != ret_ok) return ret;
			props->error_code = val;
		}
	}

	/* Post checks
	 */
	if (props->error_code == http_unset) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_HANDLER_CUSTOM_ERROR_HTTP);
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_handler_custom_error_new  (cherokee_handler_t     **hdl,
                                    void                    *cnt,
                                    cherokee_module_props_t *props)
{
	UNUSED(hdl);

	CONN(cnt)->error_code = PROP_CUSTOM_ERROR(props)->error_code;
	return ret_error;
}


