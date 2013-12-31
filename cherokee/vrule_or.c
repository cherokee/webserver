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
#include "vrule_or.h"
#include "plugin_loader.h"
#include "connection.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "vrule,or"

PLUGIN_INFO_VRULE_EASIEST_INIT(v_or);


static ret_t
match (cherokee_vrule_or_t   *vrule,
       cherokee_buffer_t     *host,
       cherokee_connection_t *conn)
{
	ret_t ret;

	/* It only needs one of the sides to match.
	 */
	ret = cherokee_vrule_match (vrule->left, host, conn);
	if (ret != ret_deny)
		return ret;

	/* It didn't match, it is time for the right side
	 */
	return cherokee_vrule_match (vrule->right, host, conn);
}


static ret_t
configure_branch (cherokee_config_node_t    *conf,
                  cherokee_virtual_server_t *vsrv,
                  const char                *branch,
                  cherokee_vrule_t         **branch_vrule)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf = NULL;

	/* Get the configuration sub-tree
	 */
	ret = cherokee_config_node_get (conf, branch, &subconf);
	if (ret != ret_ok)
		return ret;

	/* Instance the sub-rule match
	 */
	ret = cherokee_virtual_server_new_vrule (vsrv, subconf, branch_vrule);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


static ret_t
configure (cherokee_vrule_or_t       *vrule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t ret;

	ret = configure_branch (conf, vsrv, "right", &vrule->right);
	if (ret != ret_ok)
		return ret;

	ret = configure_branch (conf, vsrv, "left", &vrule->left);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


static ret_t
_free (void *p)
{
	ret_t                ret;
	cherokee_boolean_t   error = false;
	cherokee_vrule_or_t *vrule = VRULE_OR(p);

	if (vrule->left) {
		ret = cherokee_vrule_free (vrule->left);
		if (ret != ret_ok) error = true;
	}

	if (vrule->right) {
		ret = cherokee_vrule_free (vrule->right);
		if (ret != ret_ok) error = true;
	}

	return (error)? ret_error : ret_ok;
}


ret_t
cherokee_vrule_v_or_new (cherokee_vrule_t **vrule)
{
	CHEROKEE_NEW_STRUCT (n, vrule_or);

	/* Parent class constructor
	 */
	cherokee_vrule_init_base (VRULE(n), PLUGIN_INFO_PTR(v_or));

	/* Virtual methods
	 */
	VRULE(n)->match     = (vrule_func_match_t) match;
	VRULE(n)->configure = (vrule_func_configure_t) configure;
	MODULE(n)->free     = (module_func_free_t) _free;

	/* Properties
	 */
	n->left  = NULL;
	n->right = NULL;

	*vrule = VRULE(n);
	return ret_ok;
}
