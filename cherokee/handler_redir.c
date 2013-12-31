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
#include "handler_redir.h"

#include "connection.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "pcre/pcre.h"
#include "regex.h"
#include "util.h"

#define ENTRIES "handler,redir"


/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (redir, http_all_methods);


/* Methods implementation
 */
static ret_t
substitute (cherokee_handler_redir_t *hdl,
            cherokee_buffer_t        *regex,
            cherokee_buffer_t        *source,
            cherokee_buffer_t        *target,
            cint_t                   *ovector,
            cint_t                    ovector_size)
{
	ret_t                  ret;
	char                  *token;
	cint_t                 offset;
	cherokee_connection_t *conn   = HANDLER_CONN(hdl);
	cherokee_buffer_t     *tmp    = THREAD_TMP_BUF2(HANDLER_THREAD(hdl));

	cherokee_buffer_clean (tmp);

	/* Replace regex matches (vserver match)
	 */
	ret = cherokee_regex_substitute (regex, &conn->host, tmp,
	                                 conn->regex_host_ovector,
	                                 conn->regex_host_ovecsize, '^');
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	/* Replace regex matches (handler)
	 */
	ret = cherokee_regex_substitute (tmp, source, target, ovector, ovector_size, '$');
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	/* Replace variables
	 */
	token = strnstr (target->buf, "${host}", target->len);
	if (token != NULL) {
		offset = token - target->buf;
		if (! cherokee_buffer_is_empty (&conn->host)) {
			cherokee_buffer_insert_buffer (target, &conn->host, offset);
			cherokee_buffer_remove_chunk (target,  offset + conn->host.len, 7);

		} else if (! cherokee_buffer_is_empty (&conn->bind->ip)) {
			cherokee_buffer_insert_buffer (target, &conn->bind->ip, offset);
			cherokee_buffer_remove_chunk (target, offset + conn->bind->ip.len, 7);

		} else {
			cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

			ret = cherokee_copy_local_address (&conn->socket, &tmp);
			if (ret == ret_ok) {
				cherokee_buffer_insert_buffer (target, &tmp, offset);
				cherokee_buffer_remove_chunk (target, offset + tmp.len, 7);
			}

			cherokee_buffer_mrproper (&tmp);
		}
	}

	return ret_ok;
}


static ret_t
match_and_substitute (cherokee_handler_redir_t *hdl)
{
	cherokee_list_t       *i;
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);
	cherokee_buffer_t     *tmp  = THREAD_TMP_BUF1(HANDLER_THREAD(hdl));

	/* Append the query string
	 */
	if ((conn->web_directory.len > 1) &&
	    (conn->options & conn_op_document_root))
	{
		cherokee_buffer_prepend_buf (&conn->request, &conn->web_directory);
	}

	if  (! cherokee_buffer_is_empty (&conn->query_string)) {
		cherokee_buffer_add_str (&conn->request, "?");
		cherokee_buffer_add_buffer (&conn->request, &conn->query_string);
	}

	/* Try to match it
	 */
	list_for_each (i, &HDL_REDIR_PROPS(hdl)->regex_list) {
		char                   *subject;
		cint_t                  subject_len;
		cint_t                  ovector[OVECTOR_LEN];
		cint_t                  rc                    = 0;
		cherokee_regex_entry_t *list                  = REGEX_ENTRY(i);

		/* The subject usually begins with a slash. Lets imagine a request "/dir/thing".
		 * If it matched with a "/dir" directory entry, the subject have to be "/thing",
		 * so the way to extract it is to remove the directory from the beginning.
		 *
		 * There is an special case though. When a connection is using the default
		 * directory ("/"), it has to remove one character less.
		 */
		if (conn->web_directory.len == 1)
			subject = conn->request.buf + (conn->web_directory.len - 1);
		else
			subject = conn->request.buf + conn->web_directory.len;

		subject_len = strlen (subject);

		/* Case 1: No conn substitution, No local regex
		 */
		if ((list->re == NULL) &&
		    (conn->regex_ovecsize == 0))
		{
			TRACE (ENTRIES, "Using conn->ovector, size=%d\n",
			       conn->regex_ovecsize);
		}

		/* Case 2: Cached conn substitution
		 */
		else if (list->re == NULL) {
			memcpy (ovector,
				conn->regex_ovector,
				OVECTOR_LEN * sizeof(cint_t));

			rc = conn->regex_ovecsize;
			TRACE (ENTRIES, "Using conn->ovector, size=%d\n", rc);
		}

		/* Case 3: Use the rule-subentry regex
		 */
		else {
			rc = pcre_exec (list->re, NULL, subject, subject_len, 0, 0, ovector, OVECTOR_LEN);
			if (rc == 0) {
				LOG_ERROR_S (CHEROKEE_ERROR_HANDLER_REGEX_GROUPS);
			}

			TRACE (ENTRIES, "subject = \"%s\" + len(\"%s\")-1=%d\n",
			       conn->request.buf, conn->web_directory.buf, conn->web_directory.len - 1);
			TRACE (ENTRIES, "pcre_exec: subject=\"%s\" -> %d\n", subject, rc);

			if (rc <= 0) {
				continue;
			}
		}

		/* Make a copy of the original request before rewrite it
		 */
		if (cherokee_buffer_is_empty (&conn->request_original)) {
			cherokee_buffer_add_buffer (&conn->request_original, &conn->request);
		}

		cherokee_buffer_clean (tmp);
		cherokee_buffer_add (tmp, subject, subject_len);

		/* Internal redirect
		 */
		if (list->hidden == true) {
			int   len;
			char *args;

			cherokee_buffer_clean (&conn->request);
			cherokee_buffer_clean (&conn->pathinfo);
			cherokee_buffer_clean (&conn->web_directory);
			cherokee_buffer_clean (&conn->local_directory);

			cherokee_buffer_ensure_size (&conn->request, conn->request.len + subject_len);
			substitute (hdl,
			            &list->subs,    /* regex str */
			            tmp,            /* source    */
			            &conn->request, /* target    */
			            ovector, rc);   /* ovector   */


			/* Arguments */
			cherokee_split_arguments (&conn->request, 0, &args, &len);
			if (len > 0) {
				cherokee_buffer_clean (&conn->query_string);
				cherokee_buffer_add (&conn->query_string, args, len);
				cherokee_buffer_drop_ending (&conn->request, len+1);
			}

			/* Non-global redirection */
			if (conn->request.buf[0] != '/') {
				cherokee_buffer_prepend_str (&conn->request, "/");
			}

			TRACE (ENTRIES, "Hidden redirect to: request=\"%s\" query_string=\"%s\"\n",
			       conn->request.buf, conn->query_string.buf);

			return ret_eagain;
		}

		/* External redirect
		 */
		cherokee_buffer_ensure_size (&conn->redirect, conn->request.len + subject_len);

		substitute (hdl,
		            &list->subs,     /* regex str */
		            tmp,             /* source    */
		            &conn->redirect, /* target    */
		            ovector, rc);    /* ovector   */

		TRACE (ENTRIES, "Redirect %s -> %s\n", conn->request_original.buf, conn->redirect.buf);

		ret = ret_ok;
		goto out;
	}

	ret = ret_ok;

out:
	if (! cherokee_buffer_is_empty (&conn->query_string)) {
		cherokee_buffer_drop_ending (&conn->request, conn->query_string.len + 1);
	}

	if ((conn->web_directory.len > 1) &&
	    (conn->options & conn_op_document_root))
	{
		cherokee_buffer_move_to_begin (&conn->request, conn->web_directory.len);
	}

	return ret;
}


ret_t
cherokee_handler_redir_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_redir);

	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(redir));

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_redir_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_redir_free;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_redir_add_headers;

	HANDLER(n)->connection  = cnt;
	HANDLER(n)->support     = hsupport_nothing;

	n->use_previous_match   = false;

	/* If there is an explitic redirection on the connection don't
	 * even bother looking in the properties. Go straight for it.
	 */
	if (! cherokee_buffer_is_empty (&CONN(cnt)->redirect)) {
		TRACE (ENTRIES, "Explicit redirection to '%s'\n", CONN(cnt)->redirect.buf);
	} else {
		if (! cherokee_list_empty (&HDL_REDIR_PROPS(n)->regex_list)) {

			/* Manage the regex rules
			 */
			ret = match_and_substitute (n);
			if (ret == ret_eagain) {
				cherokee_handler_free (HANDLER(n));
				return ret_eagain;
			}
		}
	}

	/* Return the new handler obj
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_redir_free (cherokee_handler_redir_t *rehdl)
{
	UNUSED(rehdl);

	return ret_ok;
}


ret_t
cherokee_handler_redir_init (cherokee_handler_redir_t *n)
{
	int                    request_end;
	char                  *request_ending;
	cherokee_connection_t *conn = HANDLER_CONN(n);

	/* Maybe ::new -> match_and_substitute() has already set
	 * this redirection
	 */
	if (! cherokee_buffer_is_empty (&conn->redirect)) {
		conn->error_code = http_moved_permanently;
		return ret_error;
	}

	/* Check if it has the URL
	 */
	if (HDL_REDIR_PROPS(n)->url.len <= 0) {
		conn->error_code = http_internal_error;
		return ret_error;
	}

	/* Try with URL directive
	 */
	request_end = (conn->request.len - conn->web_directory.len);
	request_ending = conn->request.buf + conn->web_directory.len;

	cherokee_buffer_ensure_size (&conn->redirect, request_end + HDL_REDIR_PROPS(n)->url.len +1);
	cherokee_buffer_add_buffer (&conn->redirect, &HDL_REDIR_PROPS(n)->url);
	cherokee_buffer_add (&conn->redirect, request_ending, request_end);

	conn->error_code = http_moved_permanently;
	return ret_ok;
}


ret_t
cherokee_handler_redir_add_headers (cherokee_handler_redir_t *rehdl, cherokee_buffer_t *buffer)
{
	UNUSED(rehdl);
	UNUSED(buffer);

	return ret_ok;
}


static ret_t
props_free (cherokee_handler_redir_props_t *props)
{
	cherokee_buffer_mrproper (&props->url);
	cherokee_regex_list_mrproper (&props->regex_list);

	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


ret_t
cherokee_handler_redir_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                           ret;
	cherokee_list_t                *i;
	cherokee_handler_redir_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n,handler_redir_props);

		cherokee_module_props_init_base (MODULE_PROPS(n),
		                                 MODULE_PROPS_FREE(props_free));

		cherokee_buffer_init (&n->url);
		INIT_LIST_HEAD (&n->regex_list);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_REDIR(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "url")) {
			cherokee_buffer_clean (&props->url);
			cherokee_buffer_add_buffer (&props->url, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "rewrite")) {
			ret = cherokee_regex_list_configure (&props->regex_list,
			                                     subconf, srv->regexs);
			if (ret != ret_ok)
				return ret;

			/* Rewrite entries were fed in a decreasing
			 * order, but they should be evaluated from
			 * the lower to the highest: Revert them now.
			 */
			cherokee_list_invert (&props->regex_list);
		}
	}

	return ret_ok;
}
