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
#include "rule_default.h"
#include "plugin_loader.h"
#include "connection.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "rule,default"

PLUGIN_INFO_RULE_EASIEST_INIT(default);


static ret_t
match (cherokee_rule_t         *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	UNUSED(rule);
	UNUSED(ret_conf);

	if (cherokee_buffer_is_empty (&conn->web_directory)) {
		cherokee_buffer_add_str (&conn->web_directory, "/");
	}

	TRACE(ENTRIES, "Match default: %s\n", "ret_ok");
	return ret_ok;
}

static ret_t
configure (cherokee_rule_default_t   *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	UNUSED(rule);
	UNUSED(conf);
	UNUSED(vsrv);

	return ret_ok;
}

static ret_t
_free (void *p)
{
	UNUSED(p);
	return ret_ok;
}

ret_t
cherokee_rule_default_new (cherokee_rule_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_default);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(default));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	*rule = n;
	return ret_ok;
}

