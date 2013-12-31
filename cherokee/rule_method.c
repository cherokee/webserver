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
#include "rule_method.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "util.h"
#include "thread.h"
#include "server-protected.h"

#define ENTRIES "rule,method"

PLUGIN_INFO_RULE_EASIEST_INIT(method);

static ret_t
configure (cherokee_rule_method_t    *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	UNUSED(vsrv);

	ret = cherokee_config_node_read (conf, "method", &tmp);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
		              RULE(rule)->priority, "method");
		return ret_error;
	}

	ret = cherokee_http_string_to_method (tmp, &rule->method);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_METHOD_UNKNOWN, tmp->buf);
		return ret;
	}

	return ret_ok;
}

static ret_t
_free (void *p)
{
	UNUSED(p);
	return ret_ok;
}

static ret_t
match (cherokee_rule_method_t  *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	UNUSED(ret_conf);

	if (conn->header.method == rule->method) {
		TRACE(ENTRIES, "Match method: %d\n", rule->method);
		return ret_ok;
	}

	TRACE(ENTRIES, "Didn't match. Conn method=%d, Rule method=%d\n",
	      conn->header.method, rule->method);
	return ret_not_found;
}

ret_t
cherokee_rule_method_new (cherokee_rule_method_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_method);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(method));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->method = http_unknown;

	*rule = n;
	return ret_ok;
}
