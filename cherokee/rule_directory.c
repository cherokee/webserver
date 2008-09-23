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
#include "rule_directory.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "rule,directory"

PLUGIN_INFO_RULE_EASIEST_INIT(directory);


static ret_t
match (cherokee_rule_directory_t *rule, cherokee_connection_t *conn)
{
	/* Not the same lenght
	 */
	if (conn->request.len < rule->directory.len) {
		TRACE(ENTRIES, "Match directory: rule=%s req=%s: (shorter) ret_not_found\n",
		      rule->directory.buf, conn->request.buf);
		return ret_not_found;
	}

	/* Does not match
	 */
	if (strncmp (rule->directory.buf, conn->request.buf, rule->directory.len) != 0) {
		TRACE(ENTRIES, "Match directory: rule=%s req=%s: (str) ret_not_found\n",
		      rule->directory.buf, conn->request.buf);
		return ret_not_found;
	}

	/* Does not match: same begining, but a longer name
	 */
	if ((conn->request.len > rule->directory.len) &&
	    (conn->request.buf[rule->directory.len] != '/'))
	{
		TRACE(ENTRIES, "Match directory: rule=%s req=%s: (str) ret_not_found\n",
		      rule->directory.buf, conn->request.buf);
		return ret_not_found;
	}

	/* Copy the web directory property
	 */
	cherokee_buffer_add_buffer (&conn->web_directory, &rule->directory);

	/* If the request is exactly the directory entry, and it
	 * doesn't end with a slash, it must be redirected. Eg:
	 *
	 * web_directory = "/blog"
	 * request       = "/blog"
	 *
	 * It must be redirected to "/blog/"
	 */
	if ((conn->request.len > 1) &&
	    (cherokee_buffer_end_char (&conn->request) != '/') &&
	    (cherokee_buffer_cmp_buf (&conn->request, &rule->directory) == 0))
	{
		cherokee_buffer_add_str (&conn->request, "/");
		cherokee_connection_set_redirect (conn, &conn->request);
		cherokee_buffer_drop_ending (&conn->request, 1);

		TRACE(ENTRIES, "Had to redirect to: %s\n", conn->redirect.buf);
		conn->error_code = http_moved_permanently;
		return ret_error;
	}

	/* Set the web_directory if needed
	 */
	if (cherokee_buffer_is_empty (&conn->web_directory)) {
		cherokee_buffer_add_str (&conn->web_directory, "/");
	}

	TRACE(ENTRIES, "Match! rule=%s req=%s web_directory=%s: ret_ok\n",
	      rule->directory.buf, conn->request.buf, conn->web_directory.buf);

	return ret_ok;
}


static ret_t
configure (cherokee_rule_directory_t *rule,
	   cherokee_config_node_t    *conf,
	   cherokee_virtual_server_t *vsrv)
{
	ret_t ret;
	
	UNUSED(vsrv);

	ret = cherokee_config_node_copy (conf, "directory", &rule->directory);
	if (ret != ret_ok) {
		PRINT_ERROR ("Rule prio=%d needs a 'directory' property\n", 
			     RULE(rule)->priority);
		return ret_error;
	}

	cherokee_fix_dirpath (&rule->directory);
	return ret_ok;
}

static ret_t
_free (void *p)
{
	cherokee_rule_directory_t *rule = RULE_DIRECTORY(p);

	cherokee_buffer_mrproper (&rule->directory);
	return ret_ok;
}

ret_t
cherokee_rule_directory_new (cherokee_rule_directory_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_directory);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(directory));
	
	/* Virtual methos
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	cherokee_buffer_init (&n->directory);

	*rule = n;
 	return ret_ok;
}
