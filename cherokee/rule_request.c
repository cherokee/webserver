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
			rule->ovector, OVECTOR_LEN);

	if (re < 0) {
		TRACE (ENTRIES, "Request \"%s\" didn't match with \"%s\"\n", 
		       conn->request.buf, rule->pattern.buf);

		ret = ret_not_found;
		goto restore;
	}	

	rule->ovecsize = re;

	TRACE (ENTRIES, "Request \"%s\" matches with \"%s\", ovecsize=%d\n", 
	       conn->request.buf, rule->pattern.buf, rule->ovecsize);

	/* Store a pointer to the rule. We might want to use
	 * rule->ovectors from handler_redir later on.
	 */
	conn->regex_match_ovector  = &rule->ovector[0];
	conn->regex_match_ovecsize = &rule->ovecsize;

	ret = ret_ok;

restore:
        if (! cherokee_buffer_is_empty (&conn->query_string)) {
                cherokee_buffer_drop_endding (&conn->request, conn->query_string.len + 1);
        }

	return ret;
}


ret_t
cherokee_rule_request_new (cherokee_rule_request_t  **rule, 
			   cherokee_buffer_t         *value,
			   cherokee_virtual_server_t *vsrv)
{
	ret_t ret;

	CHEROKEE_NEW_STRUCT (n, rule_request);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(request));
	
	/* Virtual methos
	 */
	RULE(n)->match = (rule_func_match_t) match;

	/* Properties
	 */
	cherokee_buffer_init (&n->pattern);
	cherokee_buffer_add_buffer (&n->pattern, value);

	n->pcre = NULL;

	ret = cherokee_regex_table_add (VSERVER_SRV(vsrv)->regexs, value->buf);
	if (ret != ret_ok) return ret;

	ret = cherokee_regex_table_get (VSERVER_SRV(vsrv)->regexs, value->buf, &n->pcre);
	if (ret != ret_ok) return ret;

	*rule = n;
 	return ret_ok;
}
