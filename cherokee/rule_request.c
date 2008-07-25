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
#include "rule_request.h"
#include "plugin_loader.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "util.h"
#include "pcre/pcre.h"

#define ENTRIES "rule,request"

PLUGIN_INFO_RULE_EASIEST_INIT(request);


static ret_t 
match (cherokee_rule_request_t *rule, cherokee_connection_t *conn)
{
	int                     re;
	ret_t                   ret;
	cherokee_regex_table_t *regexs = CONN_SRV(conn)->regexs;

	/* Sanity checks
	 */
	if (unlikely (regexs == NULL)) {
		PRINT_ERROR_S ("Couldn't access regex table\n");
		return ret_error;
	}

	if (unlikely (rule->pcre == NULL)) {
		PRINT_ERROR_S ("RegExp rule has null pcre\n");
		return ret_error;
	}

        /* Build the request string
         */
        if (! cherokee_buffer_is_empty (&conn->query_string)) {
                cherokee_buffer_add_str (&conn->request, "?");
                cherokee_buffer_add_buffer (&conn->request, &conn->query_string);
        }

	/* Evaluate the pcre
	 */
	re = pcre_exec (rule->pcre, NULL, 
			conn->request.buf, 
			conn->request.len, 
			0, 0, 
			conn->regex_ovector, OVECTOR_LEN);

	if (re < 0) {
		TRACE (ENTRIES, "Request \"%s\" didn't match with \"%s\"\n", 
		       conn->request.buf, rule->pattern.buf);

		ret = ret_not_found;
		goto restore;
	}	

	conn->regex_ovecsize = re;

	TRACE (ENTRIES, "Request \"%s\" matches with \"%s\", ovecsize=%d\n", 
	       conn->request.buf, rule->pattern.buf, conn->regex_ovecsize);

	ret = ret_ok;

restore:
        if (! cherokee_buffer_is_empty (&conn->query_string)) {
                cherokee_buffer_drop_ending (&conn->request, conn->query_string.len + 1);
        }

	return ret;
}


static ret_t 
configure (cherokee_rule_request_t   *rule, 
	   cherokee_config_node_t    *conf, 
	   cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_regex_table_t *regexs = VSERVER_SRV(vsrv)->regexs;

	/* Read the configuration entry
	 */
	ret = cherokee_config_node_copy (conf, "request", &rule->pattern);
	if (ret != ret_ok) {
		PRINT_ERROR ("Rule prio=%d needs a 'request' property\n", 
			     RULE(rule)->priority);
		return ret_error;
	}

	/* Add it to the regular extension table
	 */
	ret = cherokee_regex_table_add (regexs, rule->pattern.buf);
	if (ret != ret_ok) 
		return ret;
	
	ret = cherokee_regex_table_get (regexs, rule->pattern.buf, &rule->pcre);
	if (ret != ret_ok) 
		return ret;

	return ret_ok;
}


static ret_t
_free (void *p)
{
	cherokee_rule_request_t *rule = RULE_REQUEST(p);

	cherokee_buffer_mrproper (&rule->pattern);
	return ret_ok;
}


ret_t
cherokee_rule_request_new (cherokee_rule_request_t  **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_request);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(request));
	
	/* Virtual methos
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->pcre = NULL;
	cherokee_buffer_init (&n->pattern);

	*rule = n;
 	return ret_ok;
}
