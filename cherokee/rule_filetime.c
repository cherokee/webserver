/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Stefan de Konink <stefan@konink.de>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
#include "bogotime.h"
#include "rule_filetime.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "util.h"
#include "thread.h"
#include "dtm.h"
#include "server-protected.h"

#define ENTRIES "rule,filetime"

PLUGIN_INFO_RULE_EASIEST_INIT(filetime);

static ret_t
configure (cherokee_rule_filetime_t    *rule,
	   cherokee_config_node_t    *conf,
	   cherokee_virtual_server_t *vsrv)
{
	ret_t              ret;
	cherokee_list_t   *i;

	UNUSED(vsrv);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "stat")) {
			if (equal_buf_str (&subconf->val, "atime")) {
				rule->type = ft_atime;
			} else if (equal_buf_str (&subconf->val, "ctime")) {
				rule->type = ft_ctime;
			} else if (equal_buf_str (&subconf->val, "mtime")) {
				rule->type = ft_mtime;
			} else {
				LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
				      RULE(rule)->priority, "type");
				return ret_error;
			}
		} else if (equal_buf_str (&subconf->key, "op")) {
			if (equal_buf_str (&subconf->val, "eq")) {
				rule->comparison = equal;
			} else if (equal_buf_str (&subconf->val, "le")) {
				rule->comparison = equalless;
			} else if (equal_buf_str (&subconf->val, "ge")) {
				rule->comparison = equalgreater;
			} else if (equal_buf_str (&subconf->val, "lenow")) {
				rule->comparison = equallessnow;
			} else if (equal_buf_str (&subconf->val, "genow")) {
				rule->comparison = equalgreaternow;
			} else {
				LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
					      RULE(rule)->priority, "comparison");
				return ret_error;
			}
		} else if (equal_buf_str (&subconf->key, "time")) {
			rule->time = atol (subconf->val.buf);
		}
	}

	return ret_ok;
}

static ret_t
_free (void *p)
{
	return ret_ok;
}

static ret_t
match (cherokee_rule_filetime_t  *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	ret_t                     ret;
	struct stat               nocache_info;
	struct stat              *info;
	cherokee_iocache_entry_t *io_entry = NULL;
	cherokee_server_t        *srv      = CONN_SRV(conn);

	cherokee_buffer_t *tmp  = THREAD_TMP_BUF1(CONN_THREAD(conn));

	/* Path base */
	cherokee_buffer_clean (tmp);

	if (ret_conf->document_root != NULL) {
		/* A previous non-final rule set a custom document root */
		cherokee_buffer_add_buffer (tmp, ret_conf->document_root);
	} else {
		cherokee_buffer_add_buffer (tmp, &conn->local_directory);
	}

	cherokee_buffer_add_str (tmp, "/");

	if (! cherokee_buffer_is_empty (&conn->web_directory)) {
		cherokee_buffer_add (tmp,
				     conn->request.buf + conn->web_directory.len,
				     conn->request.len - conn->web_directory.len);
	} else {
		cherokee_buffer_add_buffer (tmp, &conn->request);
	}
	
	ret = cherokee_io_stat (srv->iocache,
				tmp,
				rule->use_iocache,
				&nocache_info,
				&io_entry,
				&info);

	
	if (ret == ret_ok) {
		time_t time;
		switch (rule->type) {
			case ft_atime:
				time = info->st_atime;
				break;
			case ft_ctime:
				time = info->st_ctime;
				break;
			case ft_mtime:
				time = info->st_mtime;
				break;
		}

		ret = ret_not_found;

		switch (rule->comparison) {
			case equal:
				if (time == rule->time) ret = ret_ok;
				break;
			case equalless:
				if (time <= rule->time) ret = ret_ok;
				break;
			case equalgreater:
				if (time >= rule->time) ret = ret_ok;
				break;
			case equallessnow:
				if (time <= (cherokee_bogonow_now + rule->time)) ret = ret_ok;
				break;
			case equalgreaternow:
				if (time >= (cherokee_bogonow_now + rule->time)) ret = ret_ok;
				break;
		}
	} else {
		ret = ret_not_found;
	}

	if (io_entry) {
		cherokee_iocache_entry_unref (&io_entry);
	}

	return ret;
}

ret_t
cherokee_rule_filetime_new (cherokee_rule_filetime_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_filetime);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(filetime));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->time              = 0;
	n->comparison	     = equalless;
	n->type              = ft_ctime;

	*rule = n;
 	return ret_ok;
}
