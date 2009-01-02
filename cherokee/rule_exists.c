/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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
#include "rule_exists.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "util.h"
#include "thread.h"
#include "server-protected.h"

#define ENTRIES "rule,exists"

PLUGIN_INFO_RULE_EASIEST_INIT(exists);

typedef struct {
	cherokee_list_t   listed;
	cherokee_buffer_t file;
} entry_t;


static ret_t
parse_value (cherokee_buffer_t *value,
	     cherokee_list_t   *files)
{
	char              *val;
	char              *tmpp;
	cherokee_buffer_t  tmp = CHEROKEE_BUF_INIT;

	TRACE(ENTRIES, "Adding exists: '%s'\n", value->buf);
	cherokee_buffer_add_buffer (&tmp, value);

	tmpp = tmp.buf;
	while ((val = strsep(&tmpp, ",")) != NULL) {
		entry_t *entry;

		TRACE(ENTRIES, "Adding exists: '%s'\n", val);

		entry = (entry_t *)malloc (sizeof(entry_t));
		if (unlikely (entry == NULL))
			return ret_nomem;

		cherokee_buffer_init (&entry->file);
		cherokee_buffer_add (&entry->file, val, strlen(val));

		INIT_LIST_HEAD (&entry->listed);
		cherokee_list_add (&entry->listed, files);
	}

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}

static ret_t 
configure (cherokee_rule_exists_t    *rule, 
	   cherokee_config_node_t    *conf, 
	   cherokee_virtual_server_t *vsrv)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	UNUSED(vsrv);

	cherokee_config_node_read_bool (conf, "iocache",   &rule->use_iocache);
	cherokee_config_node_read_bool (conf, "match_any", &rule->match_any);
	
	if (rule->match_any == false) {
		ret = cherokee_config_node_read (conf, "exists", &tmp);
		if (ret != ret_ok) {
			PRINT_ERROR ("Rule prio=%d needs an 'exists' property\n", 
				     RULE(rule)->priority);
			return ret_error;
		}

		ret = parse_value (tmp, &rule->files);
		if (ret != ret_ok)
			return ret;
	}
	
	return ret_ok;
}

static ret_t
_free (void *p)
{
	cherokee_list_t        *i, *j;
	cherokee_rule_exists_t *rule = RULE_EXISTS(p);

	list_for_each_safe (i, j, &rule->files) {
		entry_t *entry = (entry_t *)i;

		cherokee_buffer_mrproper (&entry->file);
		free (entry);
	}

	return ret_ok;
}

static ret_t
match_file (cherokee_rule_exists_t *rule,
	    cherokee_connection_t  *conn,
	    cherokee_buffer_t      *fullpath)
{
	ret_t                     ret;
	struct stat               nocache_info;
	struct stat              *info;
	cherokee_iocache_entry_t *io_entry = NULL;
	cherokee_server_t        *srv      = CONN_SRV(conn);

	ret = cherokee_io_stat (srv->iocache, 
				fullpath, 
				rule->use_iocache,
				&nocache_info,
				&io_entry,
				&info);

	if (io_entry)
		cherokee_iocache_entry_unref (&io_entry);

	if (ret == ret_ok) {
		TRACE(ENTRIES, "Match exists: '%s'\n", fullpath->buf);
		return ret_ok;
	}
	
	TRACE(ENTRIES, "Rule exists: did not match '%s'\n", fullpath->buf);
	return ret_not_found;
}

static ret_t 
match (cherokee_rule_exists_t *rule,
       cherokee_connection_t  *conn)
{
	int                re;
	ret_t              ret;
	cherokee_list_t   *i;
	cherokee_buffer_t *tmp  = THREAD_TMP_BUF1(CONN_THREAD(conn));
	
	/* Path base */
	cherokee_buffer_clean      (tmp);
	cherokee_buffer_add_buffer (tmp, &CONN_VSRV(conn)->root);
	cherokee_buffer_add_str    (tmp, "/");

	/* Always match */
	if (rule->match_any) {
		cherokee_buffer_add_buffer (tmp, &conn->request);
		TRACE(ENTRIES, "Gonna match any file: '%s'\n", tmp->buf);

		return match_file (rule, conn, tmp);
	}

	/* Check the list of files */
	list_for_each (i, &rule->files) {
		entry_t *entry = (entry_t *)i;

		/* Is the request targeting this file?
		 */
		if (entry->file.len + 1 > conn->request.len)
			continue;
		
		if (conn->request.buf[(conn->request.len - entry->file.len) -1] != '/')
			continue;

		re = strncmp (entry->file.buf,
			      &conn->request.buf[conn->request.len - entry->file.len],
			      entry->file.len);
		if (re != 0)
			continue;

		/* Check whether the file exists
		 */
		cherokee_buffer_add_buffer (tmp, &entry->file);

		ret = match_file (rule, conn, tmp);
		if (ret == ret_ok)
			return ret_ok;

		cherokee_buffer_drop_ending (tmp, entry->file.len);
	}
	
	return ret_not_found;
}

ret_t
cherokee_rule_exists_new (cherokee_rule_exists_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_exists);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(exists));
	
	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	INIT_LIST_HEAD (&n->files);
	n->use_iocache = false;
	n->match_any   = false;

	*rule = n;
 	return ret_ok;
}
