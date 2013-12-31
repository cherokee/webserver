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
#include "vrule_rehost.h"
#include "plugin_loader.h"
#include "server-protected.h"
#include "match.h"
#include "regex.h"
#include "util.h"

#define ENTRIES "vrule,rehost"

PLUGIN_INFO_VRULE_EASIEST_INIT(rehost);


static ret_t
match (cherokee_vrule_rehost_t *vrule,
       cherokee_buffer_t       *host,
       cherokee_connection_t   *conn)
{
	int              re;
	cherokee_list_t *i;

	UNUSED(conn);

	list_for_each (i, &vrule->pcre_list) {
		pcre *regex = LIST_ITEM_INFO(i);

		re = pcre_exec (regex, NULL,
		                host->buf,
		                host->len,
		                0, 0,
		                conn->regex_host_ovector, OVECTOR_LEN);
		if (re >= 0) {
			conn->regex_host_ovecsize = re;
			TRACE (ENTRIES, "Host \"%s\" matched: %d variables\n", host->buf, re);
			return ret_ok;
		}
	}

	TRACE (ENTRIES, "Host \"%s\" didn't match.\n", host->buf);
	return ret_deny;
}

static ret_t
configure (cherokee_vrule_rehost_t   *vrule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_list_t        *i;
	cherokee_config_node_t *subconf;

	ret = cherokee_config_node_get (conf, "regex", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VRULE_REHOST_NO_DOMAIN, vsrv->name.buf);
		return ret_error;
	}

	list_for_each (i, &subconf->child) {
		void                   *pcre;
		cherokee_config_node_t *conf2 = CONFIG_NODE(i);

		ret = cherokee_regex_table_get (VSERVER_SRV(vsrv)->regexs, conf2->val.buf, &pcre);
		if (ret != ret_ok) {
			return ret_error;
		}

		cherokee_list_add_tail_content (&vrule->pcre_list, pcre);
	}

	return ret_ok;
}


static ret_t
_free (cherokee_vrule_rehost_t *vrule)
{
	cherokee_list_content_free (&vrule->pcre_list, NULL);
	return ret_ok;
}

ret_t
cherokee_vrule_rehost_new (cherokee_vrule_t **vrule)
{
	CHEROKEE_NEW_STRUCT (n, vrule_rehost);

	/* Parent class constructor
	 */
	cherokee_vrule_init_base (VRULE(n), PLUGIN_INFO_PTR(rehost));

	/* Virtual methods
	 */
	VRULE(n)->match     = (vrule_func_match_t) match;
	VRULE(n)->configure = (vrule_func_configure_t) configure;
	MODULE(n)->free     = (module_func_free_t) _free;

	/* Properties
	 */
	INIT_LIST_HEAD (&n->pcre_list);

	*vrule = VRULE(n);
	return ret_ok;
}

