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
#include "rule.h"
#include "connection.h"
#include "virtual_server.h"
#include "util.h"

#define ENTRIES "rule"

ret_t
cherokee_rule_init_base (cherokee_rule_t *rule, cherokee_plugin_info_t *info)
{
	cherokee_module_init_base (MODULE(rule), NULL, info);
	INIT_LIST_HEAD (&rule->list_node);

	rule->match       = NULL;
	rule->final       = true;
	rule->priority    = CHEROKEE_RULE_PRIO_NONE;
	rule->parent_rule = NULL;

	cherokee_config_entry_init (&rule->config);
	return ret_ok;
}


ret_t
cherokee_rule_free (cherokee_rule_t *rule)
{
	/* Free the Config Entry property
	 */
	cherokee_config_entry_mrproper (&rule->config);

	/* Call the virtual method
	 */
	if (MODULE(rule)->free) {
		MODULE(rule)->free (rule);
	}

	/* Free the rule
	 */
	free (rule);
	return ret_ok;
}


static ret_t
configure_base (cherokee_rule_t           *rule,
                cherokee_config_node_t    *conf)
{
	ret_t              ret;
	cherokee_buffer_t *final = NULL;

	/* Set the final value
	 */
	ret = cherokee_config_node_read (conf, "final", &final);
	if (ret == ret_ok) {
		ret = cherokee_atob (final->buf, &rule->final);
		if (ret != ret_ok) return ret_error;

		TRACE(ENTRIES, "Rule prio=%d set final to %d\n",
		      rule->priority, rule->final);
	}

	return ret_ok;
}


ret_t
cherokee_rule_configure (cherokee_rule_t *rule, cherokee_config_node_t *conf, void *vsrv)
{
	ret_t ret;

	return_if_fail (rule, ret_error);
	return_if_fail (rule->configure, ret_error);

	ret = configure_base (rule, conf);
	if (ret != ret_ok) return ret;

	/* Call the real method
	 */
	return rule->configure (rule, conf, VSERVER(vsrv));
}


ret_t
cherokee_rule_match (cherokee_rule_t *rule, void *cnt, void *ret_conf)
{
	return_if_fail (rule, ret_error);
	return_if_fail (rule->match, ret_error);

	/* Call the real method
	 */
	return rule->match (rule, CONN(cnt), CONF_ENTRY(ret_conf));
}


void
cherokee_rule_get_config (cherokee_rule_t          *rule,
                          cherokee_config_entry_t **config)
{
	cherokee_rule_t *r = rule;

	while (r->parent_rule != NULL) {
		r = r->parent_rule;
	}

	*config = &r->config;
}
