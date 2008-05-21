/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
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

	rule->match    = NULL;
	rule->final    = true;
	rule->priority = CHEROKEE_RULE_PRIO_NONE;

	cherokee_config_entry_init (&rule->config);
	return ret_ok;
}


ret_t 
cherokee_rule_free (cherokee_rule_t *rule)
{
	/* Sanity checks
	 */
	return_if_fail (rule != NULL, ret_error);

	/* Free the Config Entry property
	 */
	cherokee_config_entry_mrproper (&rule->config);

	/* Call the virtual method
	 */
	if (MODULE(rule)->free == NULL) {
		return ret_error;
	}
	
	MODULE(rule)->free (rule); 

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
		rule->final = !! atoi(final->buf);
		TRACE(ENTRIES, "Rule prio=%d set final to %d\n", 
		      rule->priority, rule->final);
	}

	return ret_ok;
}


ret_t 
cherokee_rule_configure (cherokee_rule_t *rule, cherokee_config_node_t *conf, void *vsrv)
{
	ret_t ret;

	/* Sanity checks
	 */	
	return_if_fail (rule != NULL, ret_error);

	if (rule->configure == NULL) {
		return ret_error;
	}

	ret = configure_base (rule, conf);
	if (ret != ret_ok) return ret;

	/* Call the real method
	 */
	return rule->configure (rule, conf, VSERVER(vsrv));
}


ret_t 
cherokee_rule_match (cherokee_rule_t *rule, void *cnt)
{
	/* Sanity checks
	 */	
	return_if_fail (rule != NULL, ret_error);

	if (rule->match == NULL) {
		return ret_error;
	}

	/* Call the real method
	 */
	return rule->match (rule, CONN(cnt));
}

