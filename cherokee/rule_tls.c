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
#include "rule_tls.h"
#include "util.h"

#include "server-protected.h"
#include "connection-protected.h"

#define ENTRIES "rule,tls"

PLUGIN_INFO_RULE_EASIEST_INIT(tls);

static ret_t
match (cherokee_rule_tls_t     *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	UNUSED(rule);
	UNUSED(ret_conf);

	if (conn->socket.is_tls == TLS) {
		TRACE (ENTRIES, "Match. It is %s.\n", "TLS");
		return ret_ok;
	}

	TRACE (ENTRIES, "Did not match. It is not %s.\n", "TLS");
	return ret_not_found;
}

static ret_t
configure (cherokee_rule_tls_t       *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	UNUSED (rule);
	UNUSED (conf);
	UNUSED (vsrv);

	return ret_ok;
}

static ret_t
_free (void *p)
{
	UNUSED(p);
	return ret_ok;
}


ret_t
cherokee_rule_tls_new (cherokee_rule_tls_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_tls);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(tls));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	*rule = n;
	return ret_ok;
}
