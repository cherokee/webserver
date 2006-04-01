/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
#include "reqs_list.h"
#include "regex.h"
#include "pcre/pcre.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "reqs"


ret_t 
cherokee_reqs_list_init (cherokee_reqs_list_t *rl)
{
	INIT_LIST_HEAD (rl);
	return ret_ok;
}


ret_t 
cherokee_reqs_list_mrproper (cherokee_reqs_list_t *rl)
{
	// TODO!
	return ret_ok;
}


ret_t 
cherokee_reqs_list_get (cherokee_reqs_list_t     *rl, 
			cherokee_buffer_t        *requested_url, 
			cherokee_config_entry_t  *plugin_entry,
			cherokee_connection_t    *conn)
{
	ret_t                   ret;
	list_t                 *i;
	list_t                 *reqs         = (list_t *)rl;
	char                   *request      = NULL;
	cint_t                  request_len  = 0;
	cherokee_buffer_t       request_tmp  = CHEROKEE_BUF_INIT;

	/* Sanity check
	 */
	if (CONN_SRV(conn)->regexs == NULL) 
		return ret_ok;

	/* Build the request string
	 */
	if (! cherokee_buffer_is_empty (&conn->query_string)) {
		/* Append the query_string at the end
		 */
		cherokee_buffer_ensure_size (&request_tmp, conn->request.len + conn->query_string.len + 1);
		cherokee_buffer_add_buffer (&request_tmp, &conn->request);
		cherokee_buffer_add (&request_tmp, "?", 1);
		cherokee_buffer_add_buffer (&request_tmp, &conn->query_string);

		request     = request_tmp.buf;
		request_len = request_tmp.len;
	} else {
		request     = conn->request.buf;
		request_len = conn->request.len;		
	}
	
	/* Try to match the request
	 */
	list_for_each (i, reqs) {
		int                          rei;
		pcre                        *re      = NULL;
		cherokee_reqs_list_entry_t  *lentry  = list_entry (i, cherokee_reqs_list_entry_t, list_entry);
		char                        *pattern = lentry->request.buf;
		cherokee_config_entry_t     *entry   = &lentry->base_entry;

		if (pattern == NULL)
			continue;
		
		ret = cherokee_regex_table_get (CONN_SRV(conn)->regexs, pattern, (void **)&re);
		if (ret != ret_ok) continue;
		
		rei = pcre_exec (re, NULL, request, request_len, 0, 0, lentry->ovector, OVECTOR_LEN);
		if (rei < 0) continue;

		TRACE (ENTRIES, "Request \"%s\" matches with \"%s\"\n", request, pattern);
		
		lentry->ovecsize      = rei;
		conn->req_matched_ref = lentry;

		cherokee_config_entry_complete (plugin_entry, entry, false);

		ret = ret_ok;
		goto restore;
	}

	ret = ret_not_found;

restore:
	cherokee_buffer_mrproper (&request_tmp);
	return ret;
}



ret_t 
cherokee_reqs_list_add  (cherokee_reqs_list_t       *rl, 
			 cherokee_reqs_list_entry_t *plugin_entry,
			 cherokee_regex_table_t     *regexs)
{
	ret_t ret;

	/* Add the new connection
	 */
	list_add (&plugin_entry->list_entry, (list_t *)rl);
	
	/* Compile the expression
	 */
	if (regexs != NULL) {
		if (! cherokee_buffer_is_empty (&plugin_entry->request)) {
			ret = cherokee_regex_table_add (regexs, plugin_entry->request.buf);
			if (ret != ret_ok) return ret;
		}
	}
	
	return ret_ok;
}
