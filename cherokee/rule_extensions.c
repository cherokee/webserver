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
#include "rule_extensions.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "rule,extensions"
#define MAGIC   0xFABADA

PLUGIN_INFO_RULE_EASIEST_INIT(extensions);


static ret_t
parse_value (cherokee_buffer_t *value, cherokee_avl_t *extensions)
{
	char              *val;
	char              *tmpp;
	cherokee_buffer_t  tmp = CHEROKEE_BUF_INIT;

	TRACE(ENTRIES, "Adding extensions: '%s'\n", value->buf);
	cherokee_buffer_add_buffer (&tmp, value);

	tmpp = tmp.buf;
	while ((val = strsep(&tmpp, ",")) != NULL) {
		TRACE(ENTRIES, "Adding extension: '%s'\n", val);
		cherokee_avl_add_ptr (extensions, val, (void *)MAGIC);
	}

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}

static ret_t 
configure (cherokee_rule_extensions_t *rule, 
	   cherokee_config_node_t     *conf, 
	   cherokee_virtual_server_t  *vsrv)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	UNUSED(vsrv);

	ret = cherokee_config_node_read (conf, "extensions", &tmp);
	if (ret != ret_ok) {
		PRINT_ERROR ("Rule prio=%d needs an 'extensions' property\n", 
			     RULE(rule)->priority);
		return ret_error;
	}

	return parse_value (tmp, &rule->extensions);
}

static ret_t
_free (void *p)
{
	cherokee_rule_extensions_t *rule = RULE_EXTENSIONS(p);

	cherokee_avl_mrproper (&rule->extensions, NULL);
	return ret_ok;
}

static ret_t 
match (cherokee_rule_extensions_t *rule, cherokee_connection_t *conn)
{
	ret_t  ret;
	char  *dot;
	void  *foo;

	dot = strrchr (conn->request.buf, '.');
	if (dot == NULL) return ret_not_found;

	ret = cherokee_avl_get_ptr (&rule->extensions, dot+1, &foo);
        if (ret != ret_ok) {
		TRACE(ENTRIES, "Rule extension: did not match '%s'\n", dot+1);
		return ret;
	}

	TRACE(ENTRIES, "Match extension: '%s'\n", dot+1);
	return ret_ok;
}

ret_t
cherokee_rule_extensions_new (cherokee_rule_extensions_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_extensions);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(extensions));
	
	/* Virtual methos
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	cherokee_avl_init (&n->extensions);

	*rule = n;
 	return ret_ok;
}
