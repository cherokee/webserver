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
#include "rule_not.h"
#include "plugin_loader.h"
#include "virtual_server.h"

#define ENTRIES "rule,not"

PLUGIN_INFO_RULE_EASIEST_INIT(not);

static ret_t
match (cherokee_rule_t         *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	ret_t ret;

	/* Call match() in the subrule and invert the result
	 */
	ret = cherokee_rule_match (RULE_NOT(rule)->right, conn, ret_conf);
	switch (ret) {
	case ret_ok:
		return ret_not_found;
	case ret_not_found:
		return ret_ok;
	case ret_error:
		return ret_error;
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}
}

static ret_t
configure (cherokee_rule_not_t       *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf = NULL;

	/* Get the configuration sub-tree
	 */
	ret = cherokee_config_node_get (conf, "right", &subconf);
	if (ret != ret_ok)
		return ret;

	/* Instance the sub-rule match
	 */
	ret = cherokee_virtual_server_new_rule (vsrv, subconf,
	                                        RULE(rule)->priority,
	                                        &rule->right);
	if (ret != ret_ok)
		return ret;

	if (rule->right == NULL)
		return ret_error;

	/* Let the child rule know about its parent
	 */
	rule->right->parent_rule = RULE(rule);

	return ret_ok;
}

static ret_t
_free (void *p)
{
	ret_t                ret;
	cherokee_boolean_t   error = false;
	cherokee_rule_not_t *rule  = RULE_NOT(p);

	if (rule->right) {
		ret = cherokee_rule_free (rule->right);
		if (ret != ret_ok) error = true;
	}

	return (error)? ret_error : ret_ok;
}

ret_t
cherokee_rule_not_new (cherokee_rule_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_not);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(not));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->right = NULL;

	*rule = RULE(n);
	return ret_ok;
}

