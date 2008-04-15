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
	return ret_ok;
}


static ret_t 
configure (cherokee_rule_header_t    *rule, 
	   cherokee_config_node_t    *conf, 
	   cherokee_virtual_server_t *vsrv)
{
	ret_t              ret;
	cherokee_buffer_t *header = NULL;

	ret = cherokee_config_node_read (conf, "header", &header);
	if (ret != ret_ok) {
		PRINT_ERROR ("Rule header prio=%d needs a header entry\n", 
			     RULE(rule)->priority);
		return ret_error;
	}

	


/* 	cherokee_buffer_add_buffer (&n->pattern, value); */

/* 	ret = cherokee_regex_table_add (VSERVER_SRV(vsrv)->regexs, value->buf); */
/* 	if (ret != ret_ok) return ret; */

/* 	ret = cherokee_regex_table_get (VSERVER_SRV(vsrv)->regexs, value->buf, &n->pcre); */
/* 	if (ret != ret_ok) return ret; */

	return ret_ok;
}


ret_t
cherokee_rule_header_new (cherokee_rule_header_t **rule)
{
	ret_t ret;

	CHEROKEE_NEW_STRUCT (n, rule_header);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(header));
	
	/* Virtual methos
	 */
	RULE(n)->match = (rule_func_match_t) match;

	/* Properties
	 */
	n->pcre = NULL;
	cherokee_buffer_init (&n->pattern);

	*rule = n;
 	return ret_ok;
}
