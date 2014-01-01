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
#include "rule_fullpath.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "util.h"
#include "thread.h"
#include "server-protected.h"

#define ENTRIES "rule,fullpath"

PLUGIN_INFO_RULE_EASIEST_INIT(fullpath);

static ret_t
configure (cherokee_rule_fullpath_t  *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf;
	cherokee_list_t        *i;

	UNUSED (vsrv);

	/* Get the entry
	 */
	ret = cherokee_config_node_get (conf, "fullpath", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
		              RULE(rule)->priority, "fullpath");
		return ret_error;
	}

	/* Add the paths
	 */
	cherokee_config_node_foreach (i, subconf) {
		cherokee_config_node_t *path = CONFIG_NODE(i);

		TRACE(ENTRIES, "Adding fullpath entry (key=%s): '%s'\n",
		      path->key.buf, path->val.buf);

		cherokee_avl_add (&rule->paths, &path->val, NULL);
	}

	return ret_ok;
}


static ret_t
_free (cherokee_rule_fullpath_t *rule)
{
	cherokee_avl_mrproper (AVL_GENERIC(&rule->paths), NULL);
	return ret_ok;
}


static ret_t
match (cherokee_rule_fullpath_t *rule,
       cherokee_connection_t    *conn,
       cherokee_config_entry_t  *ret_conf)
{
	ret_t ret;

	UNUSED (conn);
	UNUSED (ret_conf);

	ret = cherokee_avl_get (&rule->paths, &conn->request, NULL);
	switch (ret) {
	case ret_ok:
		TRACE(ENTRIES, "Rule fullpath match: '%s'\n", conn->request.buf);
		return ret_ok;

	case ret_not_found:
		TRACE(ENTRIES, "Rule fullpath: did not match '%s'\n", conn->request.buf);
		return ret_not_found;

	default:
		conn->error_code = http_internal_error;
		return ret_error;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
cherokee_rule_fullpath_new (cherokee_rule_fullpath_t **rule)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, rule_fullpath);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(fullpath));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	ret = cherokee_avl_init (&n->paths);
	if (ret != ret_ok) {
		return ret;
	}

	*rule = n;
	return ret_ok;
}

