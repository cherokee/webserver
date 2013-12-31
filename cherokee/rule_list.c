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
#include "rule_list.h"

#include "util.h"
#include "connection-protected.h"
#include "rule_default.h"

#define ENTRIES "rules"


ret_t
cherokee_rule_list_init (cherokee_rule_list_t *list)
{
	ret_t ret;

	INIT_LIST_HEAD (&list->rules);

	ret = cherokee_rule_default_new (&list->def_rule);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


ret_t
cherokee_rule_list_mrproper (cherokee_rule_list_t *list)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, &list->rules) {
		cherokee_rule_t *rule = list_entry (i, cherokee_rule_t, list_node);
		cherokee_rule_free (rule);
	}

	cherokee_rule_free (list->def_rule);
	return ret_ok;
}

static void
update_connection (cherokee_connection_t   *conn,
                   cherokee_config_entry_t *ret_config)
{
	if (! conn->auth_type)
		conn->auth_type = ret_config->authentication;

	if ((conn->expiration       == cherokee_expiration_none) &&
	    (ret_config->expiration != cherokee_expiration_none))
	{
		conn->expiration      = ret_config->expiration;
		conn->expiration_time = ret_config->expiration_time;
		conn->expiration_prop = ret_config->expiration_prop;
	}

	if (! NULLI_IS_NULL(ret_config->timeout_lapse)) {
		conn->timeout_lapse  = ret_config->timeout_lapse;
		conn->timeout_header = ret_config->timeout_header;
		cherokee_connection_update_timeout (conn);
	}
}

ret_t
cherokee_rule_list_match (cherokee_rule_list_t    *list,
                          cherokee_connection_t   *conn,
                          cherokee_config_entry_t *ret_config)
{
	ret_t            ret;
	cherokee_list_t *i;
	cherokee_rule_t *rule;

	list_for_each (i, &list->rules) {
		rule = list_entry (i, cherokee_rule_t, list_node);

		TRACE(ENTRIES, "Trying rule prio=%d\n", rule->priority);

		/* Does this rule apply
		 */
		ret = cherokee_rule_match (rule, conn, ret_config);
		switch (ret) {
		case ret_not_found:
			continue;
		case ret_ok:
			break;
		case ret_error:
			return ret_error;
		default:
			RET_UNKNOWN(ret);
		}

		/* It has matched: apply the config
		 */
		TRACE(ENTRIES, "Merging rule prio=%d\n", rule->priority);
		cherokee_config_entry_complete (ret_config, &rule->config);

		/* Should it continue? */
		if (rule->final) {
			goto out;
		}
	}

	TRACE(ENTRIES, "Did not match any. Using %s\n", "default");

	list->def_rule->match(list->def_rule, conn, ret_config);
	cherokee_config_entry_complete (ret_config, &list->def_rule->config);

out:
	/* Update the connection
	 */
	update_connection (conn, ret_config);
	return ret_ok;
}


ret_t
cherokee_rule_list_add (cherokee_rule_list_t *list, cherokee_rule_t *rule)
{
	cherokee_list_add_tail (&rule->list_node, &list->rules);
	return ret_ok;
}


static int
rule_cmp (cherokee_list_t *a, cherokee_list_t *b)
{
	cherokee_rule_t *A = list_entry (a, cherokee_rule_t, list_node);
	cherokee_rule_t *B = list_entry (b, cherokee_rule_t, list_node);

	return (B->priority - A->priority);
}


ret_t
cherokee_rule_list_sort (cherokee_rule_list_t *list)
{
#ifdef TRACE_ENABLED
	{
		cherokee_list_t *i;
		cuint_t          n = 0;
		list_for_each (i, &list->rules) { n++; }
		TRACE(ENTRIES, "Sorting %d rules\n", n);
	}
#endif

	cherokee_list_sort (&list->rules, rule_cmp);
	return ret_ok;
}
