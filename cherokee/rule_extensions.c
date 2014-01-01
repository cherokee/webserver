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
#include "rule_extensions.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "server-protected.h"
#include "thread.h"
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
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
		              RULE(rule)->priority, "extensions");
		return ret_error;
	}

	cherokee_config_node_read_bool (conf, "check_local_file", &rule->check_local_file);
	cherokee_config_node_read_bool (conf, "iocache",          &rule->use_iocache);

	return parse_value (tmp, &rule->extensions);
}

static ret_t
_free (void *p)
{
	cherokee_rule_extensions_t *rule = RULE_EXTENSIONS(p);

	cherokee_avl_mrproper (AVL_GENERIC(&rule->extensions), NULL);
	return ret_ok;
}

static ret_t
local_file_exists (cherokee_rule_extensions_t *rule,
                   cherokee_connection_t      *conn,
                   cherokee_config_entry_t    *ret_conf)
{
	ret_t                     ret;
	struct stat              *info;
	struct stat               nocache_info;
	cherokee_boolean_t        is_file;
	cherokee_iocache_entry_t *io_entry      = NULL;
	cherokee_server_t        *srv           = CONN_SRV(conn);
	cherokee_buffer_t        *tmp           = THREAD_TMP_BUF1(CONN_THREAD(conn));

	UNUSED(rule);

	/* Build the full path
	 */
	cherokee_buffer_clean (tmp);

	if (ret_conf->document_root != NULL) {
		/* A previous non-final rule set a custom document root */
		cherokee_buffer_add_buffer (tmp, ret_conf->document_root);
	} else {
		cherokee_buffer_add_buffer (tmp, &conn->local_directory);
	}

	if (! cherokee_buffer_is_empty (&conn->web_directory)) {
		cherokee_buffer_add (tmp,
		                     conn->request.buf + conn->web_directory.len,
		                     conn->request.len - conn->web_directory.len);
	} else {
		cherokee_buffer_add_buffer (tmp, &conn->request);
	}

	/* Check the local file
	 */
	ret = cherokee_io_stat (srv->iocache, tmp, rule->use_iocache,
	                        &nocache_info, &io_entry, &info);

	if (ret == ret_ok) {
		is_file = S_ISREG(info->st_mode);
	}

	if (io_entry) {
		cherokee_iocache_entry_unref (&io_entry);
	}

	/* Report and return
	 */
	if (ret != ret_ok) {
		TRACE(ENTRIES, "Rule extensions: almost matched '%s', but file does not exist\n", tmp->buf);
		return ret_not_found;
	}

	if (! is_file) {
		TRACE(ENTRIES, "Rule extensions: almost matched '%s', but it is not a file\n", tmp->buf);
		return ret_not_found;
	}

	return ret_ok;
}

static ret_t
match (cherokee_rule_extensions_t *rule,
       cherokee_connection_t      *conn,
       cherokee_config_entry_t    *ret_conf)
{
	ret_t  ret;
	char  *dot;
	char  *slash;
	char  *end;
	char  *p;
	void  *foo;
	char  *dot_prev = NULL;

	UNUSED(ret_conf);

	end = conn->request.buf + conn->request.len;
	p   = end - 1;

	/* For each '.' */
	while (p > conn->request.buf) {
		if (*p != '.') {
			p--;
			continue;
		}

		if (p[1] == '\0') {
			p--;
			continue;
		}

		if (p[1] == '/') {
			p--;
			continue;
		}

		dot   = p;
		slash = NULL;

		/* Find a slash after the dot */
		while (p < end) {
			if (*p == '/') {
				slash = p;
				*p = '\0';
				break;
			}

			p++;

			if ((dot_prev != NULL) && (p >= dot_prev)) {
				break;
			}
		}

		/* Check it out */
		ret = cherokee_avl_get_ptr (&rule->extensions, dot+1, &foo);
		switch (ret) {
		case ret_ok:
			if (rule->check_local_file) {
				ret = local_file_exists (rule, conn, ret_conf);
				if (ret != ret_ok) {
					break;
				}
			}

			TRACE(ENTRIES, "Match extension: '%s'\n", dot+1);
			if (slash != NULL) {
				*slash = '/';
			}
			return ret_ok;
		case ret_not_found:
			TRACE(ENTRIES, "Rule extension: did not match '%s'\n", dot+1);
			break;
		default:
			conn->error_code = http_internal_error;
			return ret_error;
		}

		/* Revert pathinfo match char
		 */
		if (slash != NULL) {
			*slash = '/';
		}

		dot_prev = dot;
		p = dot - 1;
	}

	TRACE(ENTRIES, "Rule extension: nothing more to test '%s'\n", conn->request.buf);
	return ret_not_found;
}


ret_t
cherokee_rule_extensions_new (cherokee_rule_extensions_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_extensions);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(extensions));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->check_local_file = false;
	n->use_iocache      = true;

	cherokee_avl_init (&n->extensions);

	*rule = n;
	return ret_ok;
}
