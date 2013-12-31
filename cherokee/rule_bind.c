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
#include "rule_bind.h"

#include "server-protected.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "rule,bind"

PLUGIN_INFO_RULE_EASIEST_INIT(bind);

static ret_t
match (cherokee_rule_bind_t    *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	cherokee_list_t *i;

	UNUSED(ret_conf);

	list_for_each (i, &rule->binds) {
		if (LIST_ITEM_INFO(i) == conn->bind) {
			TRACE(ENTRIES, "Match bind num=%d\n", conn->bind->id);
			return ret_ok;
		}
	}

	TRACE(ENTRIES, "Rule bind: num=%d did not match any\n", conn->bind->id);
	return ret_not_found;
}

static ret_t
configure (cherokee_rule_bind_t      *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_list_t        *i;
	cherokee_config_node_t *subconf;
	cherokee_server_t      *srv      = VSERVER_SRV(vsrv);

	UNUSED(vsrv);

	ret = cherokee_config_node_get (conf, "bind", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY, RULE(rule)->priority, "bind");
		return ret_error;
	}

	cherokee_config_node_foreach (i, subconf) {
		int                     bind_n;
		cherokee_list_t        *bind_obj;
		cherokee_config_node_t *subconf2 = CONFIG_NODE(i);

		ret = cherokee_atoi (subconf2->val.buf, &bind_n);
		if (ret != ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_RULE_BIND_PORT,
				      RULE(rule)->priority, subconf2->val.buf);
			return ret_error;
		}

		list_for_each (bind_obj, &srv->listeners){
			if (BIND(bind_obj)->id != bind_n)
				continue;

			TRACE(ENTRIES, "Adding rule bind: %d\n", bind_n);

			ret = cherokee_list_add_tail_content (&rule->binds, bind_obj);
			if (ret != ret_ok)
				return ret_error;
			break;
		}

	}

	return ret_ok;
}

static ret_t
_free (void *p)
{
	cherokee_rule_bind_t *rule = RULE_BIND(p);

	cherokee_list_content_free (&rule->binds, NULL);
	return ret_ok;
}


ret_t
cherokee_rule_bind_new (cherokee_rule_bind_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_bind);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(bind));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	INIT_LIST_HEAD(&n->binds);

	*rule = n;
	return ret_ok;
}
