/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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
#include "rule_http_arg.h"

#include "plugin_loader.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "util.h"
#include "pcre/pcre.h"

#define ENTRIES "rule,http_arg"

PLUGIN_INFO_RULE_EASIEST_INIT(http_arg);


static ret_t
check_argument (cherokee_rule_http_arg_t *rule,
		cherokee_buffer_t        *value)
{
	int re;

	/* Check whether it matches
	 */
	re = pcre_exec (rule->pcre, NULL,
			value->buf, value->len,
			0, 0, NULL, 0);

	if (re < 0) {
		TRACE (ENTRIES, "Parameter value '%s' didn't match with '%s'\n", 
		       value->buf, rule->match.buf);
		return ret_not_found;
	}

	TRACE (ENTRIES, "Parameter value '%s' matched with '%s'\n", 
	       value->buf, rule->match.buf);

	return ret_ok;
}


static ret_t
match_avl_func (cherokee_buffer_t *key, void *value, void *param)
{
	ret_t                     ret;
	cherokee_buffer_t         fake;
	cherokee_rule_http_arg_t *rule = RULE_HTTP_ARG(param);

	UNUSED(key);

	// Temporal until conn->arguments uses cherokee_buffer_t
	cherokee_buffer_fake (&fake, (char *)value, strlen((char *)value));

	ret = check_argument (rule, &fake);
	if (ret == ret_not_found) {
		return ret_ok;
	}

	/* Found */
	return ret_eof;
}


static ret_t 
match (cherokee_rule_http_arg_t *rule, 
       cherokee_connection_t    *conn,
       cherokee_config_entry_t  *ret_conf)
{
	ret_t ret;

	UNUSED(ret_conf);

	/* Parse HTTP arguments
	 */
	ret = cherokee_connection_parse_args (conn);
	if (ret != ret_ok) {
		return ret_not_found;
	}

	if (conn->arguments == NULL) {
		return ret_not_found;
	}

	/* Retrieve the right one
	 */
	if (! cherokee_buffer_is_empty (&rule->arg)) {
		cherokee_buffer_t  fake;
		char              *value;
		
		ret = cherokee_avl_get (conn->arguments, &rule->arg, (void **)&value);
		if (ret != ret_ok) {
			return ret_not_found;
		}

		// Temporal until conn->arguments uses cherokee_buffer_t
		cherokee_buffer_fake (&fake, (char *)value, strlen((char *)value));

		return check_argument (rule, &fake);
	} 

	/* Check all arguments
	 */
	else {
		ret = cherokee_avl_while (conn->arguments, match_avl_func, rule, NULL, NULL);
		if (ret == ret_eof) {
			return ret_ok;
		}
	}

	return ret_not_found;
}


static ret_t 
configure (cherokee_rule_http_arg_t  *rule, 
	   cherokee_config_node_t    *conf, 
	   cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_regex_table_t *regexs   = VSERVER_SRV(vsrv)->regexs;

	/* Read the matching reg-ex
	 */
	ret = cherokee_config_node_copy (conf, "match", &rule->match);
	if (ret != ret_ok) {
		LOG_ERROR (CHEROKEE_ERROR_RULE_NO_PROPERTY,
			   RULE(rule)->priority, "match");
		return ret_error;
	}

	/* Read the optional argument
	 */
	cherokee_config_node_copy (conf, "arg", &rule->arg);

	/* Compile the regular expression
	 */
	ret = cherokee_regex_table_add (regexs, rule->match.buf);
	if (ret != ret_ok) 
		return ret;
	
	ret = cherokee_regex_table_get (regexs, rule->match.buf, &rule->pcre);
	if (ret != ret_ok) 
		return ret;

	return ret_ok;
}


static ret_t
_free (void *p)
{
	cherokee_rule_http_arg_t *rule = RULE_HTTP_ARG(p);

	cherokee_buffer_mrproper (&rule->arg);
	cherokee_buffer_mrproper (&rule->match);
	return ret_ok;
}


ret_t
cherokee_rule_http_arg_new (cherokee_rule_http_arg_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_http_arg);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(http_arg));
	
	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->pcre = NULL;
	cherokee_buffer_init (&n->arg);
	cherokee_buffer_init (&n->match);

	*rule = n;
 	return ret_ok;
}
