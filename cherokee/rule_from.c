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
#include "rule_from.h"

#include "server-protected.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "rule,from"

PLUGIN_INFO_RULE_EASIEST_INIT(from);


static ret_t
match (cherokee_rule_from_t    *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	ret_t ret;

	UNUSED(ret_conf);

	ret = cherokee_access_ip_match (&rule->access, &conn->socket);
	if (ret != ret_ok) {
		TRACE(ENTRIES, "Rule from did %s match any\n", "not");
		return ret_not_found;
	}

	TRACE(ENTRIES, "Rule from matched %s", "\n");
	return ret_ok;
}

static ret_t
configure (cherokee_rule_from_t      *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_list_t        *i;
	cherokee_config_node_t *subconf;

	UNUSED(vsrv);

	ret = cherokee_config_node_get (conf, "from", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
		              RULE(rule)->priority, "from");
		return ret_error;
	}

	cherokee_config_node_foreach (i, subconf) {
		cherokee_config_node_t *subconf2 = CONFIG_NODE(i);

		ret = cherokee_access_add (&rule->access, subconf2->val.buf);
		if (ret != ret_ok) {
			LOG_ERROR (CHEROKEE_ERROR_RULE_FROM_ENTRY, subconf2->val.buf);
			return ret_error;
		}
	}

	return ret_ok;
}

static ret_t
_free (void *p)
{
	cherokee_rule_from_t *rule = RULE_FROM(p);

	cherokee_access_mrproper (&rule->access);
	return ret_ok;
}


ret_t
cherokee_rule_from_new (cherokee_rule_from_t **rule)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, rule_from);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(from));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	ret = cherokee_access_init (&n->access);
	if (ret != ret_ok) return ret;

	*rule = n;
	return ret_ok;
}
