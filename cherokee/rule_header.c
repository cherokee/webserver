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
#include "rule_header.h"

#include "plugin_loader.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "util.h"
#include "pcre/pcre.h"

#define ENTRIES "rule,header"

PLUGIN_INFO_RULE_EASIEST_INIT(header);


static ret_t 
match (cherokee_rule_header_t *rule, 
       cherokee_connection_t  *conn)
{
	int      re;
	ret_t    ret;
	char    *info     = NULL;
	cuint_t  info_len = 0;

	/* Find the header
	 */
	ret = cherokee_header_get_known (&conn->header,
					 rule->header,
					 &info, &info_len);

	if ((ret != ret_ok) || (info == NULL)) {
		TRACE (ENTRIES, "Request '%s'; couldn't find header(%d)\n", 
		       conn->request.buf, rule->header);
		return ret_not_found;
	}

	/* Check whether it matches
	 */
	re = pcre_exec (rule->pcre, NULL, 
			info, info_len,
			0, 0, NULL, 0);

	if (re < 0) {
		TRACE (ENTRIES, "Request '%s' didn't match header(%d) with '%s'\n", 
		       conn->request.buf, rule->header, rule->match.buf);
		return ret_not_found;
	}

	TRACE (ENTRIES, "Request '%s' matched header(%d) with '%s'\n", 
	       conn->request.buf, rule->header, rule->match.buf);
	return ret_ok;
}


static ret_t
header_str_to_type (cherokee_buffer_t        *header,
		    cherokee_common_header_t *common_header)
{
	if (equal_buf_str (header, "Accept-Encoding")) {
		*common_header = header_accept_encoding;
	} else if (equal_buf_str (header, "Accept-Charset")) {
		*common_header = header_accept_charset;
	} else if (equal_buf_str (header, "Accept-Language")) {
		*common_header = header_accept_language;
	} else if (equal_buf_str (header, "Referer")) {
		*common_header = header_referer;
	} else if (equal_buf_str (header, "User-Agent")) {
		*common_header = header_user_agent;
	} else {
		PRINT_ERROR ("ERROR: Unknown header: '%s'\n", header->buf);
		return ret_error;
	}

	return ret_ok;
}


static ret_t 
configure (cherokee_rule_header_t    *rule, 
	   cherokee_config_node_t    *conf, 
	   cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_buffer_t      *header   = NULL;
	cherokee_regex_table_t *regexs   = VSERVER_SRV(vsrv)->regexs;

	/* Read the header
	 */
	ret = cherokee_config_node_read (conf, "header", &header);
	if (ret != ret_ok) {
		PRINT_ERROR ("Rule header prio=%d needs a 'header' entry\n", 
			     RULE(rule)->priority);
		return ret_error;
	}

	ret = header_str_to_type (header, &rule->header);
	if (ret != ret_ok) 
		return ret;
	
	/* Read the match
	 */
	ret = cherokee_config_node_copy (conf, "match", &rule->match);
	if (ret != ret_ok) {
		PRINT_ERROR ("Rule header prio=%d needs a 'match' entry\n", 
			     RULE(rule)->priority);
		return ret_error;
	}

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
	cherokee_rule_header_t *rule = RULE_HEADER(p);

	cherokee_buffer_mrproper (&rule->match);
	return ret_ok;
}


ret_t
cherokee_rule_header_new (cherokee_rule_header_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_header);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(header));
	
	/* Virtual methos
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->pcre = NULL;
	cherokee_buffer_init (&n->match);

	*rule = n;
 	return ret_ok;
}
