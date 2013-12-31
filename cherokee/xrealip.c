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
#include "xrealip.h"


ret_t
cherokee_x_real_ip_init (cherokee_x_real_ip_t *real_ip)
{
	real_ip->enabled    = false;
	real_ip->access     = NULL;
	real_ip->access_all = false;

	return ret_ok;
}


ret_t
cherokee_x_real_ip_mrproper (cherokee_x_real_ip_t *real_ip)
{
	if (real_ip->access != NULL) {
		cherokee_access_free (real_ip->access);
		real_ip->access = NULL;
	}

	return ret_ok;
}


static ret_t
add_access (char *address, void *data)
{
	ret_t                 ret;
	cherokee_x_real_ip_t *real_ip = X_REAL_IP(data);

	if (real_ip->access == NULL) {
		ret = cherokee_access_new (&real_ip->access);
		if (ret != ret_ok) {
			return ret;
		}
	}

	ret = cherokee_access_add (real_ip->access, address);
	if (ret != ret_ok) {
		return ret;
	}

	return ret_ok;
}


ret_t
cherokee_x_real_ip_configure (cherokee_x_real_ip_t   *real_ip,
                              cherokee_config_node_t *config)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf;

	cherokee_config_node_read_bool (config, "x_real_ip_enabled",    &real_ip->enabled);
	cherokee_config_node_read_bool (config, "x_real_ip_access_all", &real_ip->access_all);

	ret = cherokee_config_node_get (config, "x_real_ip_access", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_config_node_read_list (subconf, NULL, add_access, real_ip);
		if (ret != ret_ok) {
			LOG_ERROR_S (CHEROKEE_ERROR_LOGGER_X_REAL_IP_PARSE);
			return ret_error;
		}
	}

	return ret_ok;
}


ret_t
cherokee_x_real_ip_is_allowed (cherokee_x_real_ip_t  *real_ip,
                               cherokee_socket_t     *sock)
{
	ret_t ret;

	if (! real_ip->access_all) {
		if (real_ip->access == NULL) {
			return ret_deny;
		}

		ret = cherokee_access_ip_match (real_ip->access, sock);
		if (ret != ret_ok) {
			return ret_deny;
		}
	}

	return ret_ok;
}
