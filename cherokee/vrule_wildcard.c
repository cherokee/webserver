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
#include "vrule_wildcard.h"
#include "plugin_loader.h"
#include "connection.h"
#include "connection-protected.h"
#include "match.h"
#include "util.h"

#define ENTRIES "vrule,wildcard"

PLUGIN_INFO_VRULE_EASIEST_INIT(wildcard);

typedef struct {
	cherokee_list_t     node;
	cherokee_buffer_t   domain;
	cherokee_boolean_t  is_wildcard;
} cherokee_wc_entry_t;


/* Wild-card entries
 */

static ret_t
entry_new (cherokee_wc_entry_t **wc,
           cherokee_buffer_t    *domain)
{
	CHEROKEE_NEW_STRUCT (n, wc_entry);

	INIT_LIST_HEAD(&n->node);

	cherokee_buffer_init (&n->domain);
	cherokee_buffer_add_buffer (&n->domain, domain);

	n->is_wildcard = (strchr (domain->buf, '*') ||
	                  strchr (domain->buf, '?'));

	*wc = n;
	return ret_ok;
}

static void
entry_free (cherokee_wc_entry_t *wc)
{
	cherokee_buffer_mrproper (&wc->domain);
	free (wc);
}


/* Plug-in
 */

static ret_t
match (cherokee_vrule_wildcard_t *vrule,
       cherokee_buffer_t         *host,
       cherokee_connection_t     *conn)
{
	int              re;
	ret_t            ret;
	cherokee_list_t *i;

	UNUSED(conn);

	list_for_each (i, &vrule->entries) {
		cherokee_wc_entry_t *entry = (cherokee_wc_entry_t *)i;

		if (entry->is_wildcard) {
			ret = cherokee_wildcard_match (entry->domain.buf, host->buf);
		} else {
			re = cherokee_buffer_cmp_buf (&entry->domain, host);
			ret = (re == 0) ? ret_ok : ret_deny;
		}

		if (ret == ret_ok) {
			TRACE(ENTRIES, "VRule wildcard: matched '%s'\n", entry->domain.buf);
			return ret_ok;
		}

		TRACE(ENTRIES, "VRule wildcard: did not match '%s'\n", entry->domain.buf);
	}

	return ret_deny;
}

static ret_t
configure (cherokee_vrule_wildcard_t   *vrule,
           cherokee_config_node_t      *conf,
           cherokee_virtual_server_t   *vsrv)
{
	ret_t                   ret;
	cherokee_list_t        *i;
	cherokee_config_node_t *subconf;

	UNUSED(vsrv);

	ret = cherokee_config_node_get (conf, "domain", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VRULE_NO_PROPERTY, vsrv->name.buf, "domain");
		return ret_error;
	}

	list_for_each (i, &subconf->child) {
		cherokee_wc_entry_t    *entry;
		cherokee_config_node_t *conf2  = CONFIG_NODE(i);

		ret = entry_new (&entry, &conf2->val);
		if (ret != ret_ok) {
			return ret_error;
		}

		cherokee_list_add_tail (&entry->node, &vrule->entries);
	}

	return ret_ok;
}

static ret_t
_free (void *p)
{
	cherokee_list_t           *i, *tmp;
	cherokee_vrule_wildcard_t *vrule    = p;

	list_for_each_safe (i, tmp, &vrule->entries) {
		entry_free ((cherokee_wc_entry_t *)i);
	}

	return ret_ok;
}

ret_t
cherokee_vrule_wildcard_new (cherokee_vrule_t **vrule)
{
	CHEROKEE_NEW_STRUCT (n, vrule_wildcard);

	/* Parent class constructor
	 */
	cherokee_vrule_init_base (VRULE(n), PLUGIN_INFO_PTR(wildcard));

	/* Virtual methods
	 */
	VRULE(n)->match     = (vrule_func_match_t) match;
	VRULE(n)->configure = (vrule_func_configure_t) configure;
	MODULE(n)->free     = (module_func_free_t) _free;

	/* Properties
	 */
	INIT_LIST_HEAD (&n->entries);

	*vrule = VRULE(n);
	return ret_ok;
}

