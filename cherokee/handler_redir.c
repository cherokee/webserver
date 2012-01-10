/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
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
#include "handler_redir.h"

#include "request.h"
#include "server-protected.h"
#include "request-protected.h"
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
	ret_t               ret;
	char               *token;
	cint_t              offset;
	cherokee_request_t *req     = HANDLER_REQ(hdl);
	cherokee_buffer_t  *tmp     = THREAD_TMP_BUF2(HANDLER_THREAD(hdl));

	cherokee_buffer_clean (tmp);

	/* Replace regex matches (vserver match)
	 */
	ret = cherokee_regex_substitute (regex, &req->host, tmp,
					 req->regex_host_ovector,
					 req->regex_host_ovecsize, '^');
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
		if (! cherokee_buffer_is_empty (&req->host)) {
			cherokee_buffer_insert_buffer (target, &req->host, offset);
			cherokee_buffer_remove_chunk (target,  offset + req->host.len, 7);

		} else if (! cherokee_buffer_is_empty (&req->bind->ip)) {
			cherokee_buffer_insert_buffer (target, &req->bind->ip, offset);
			cherokee_buffer_remove_chunk (target, offset + req->bind->ip.len, 7);

		} else {
			cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

			ret = cherokee_copy_local_address (&req->socket, &tmp);
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
	cherokee_list_t    *i;
	ret_t               ret;
	cherokee_request_t *req  = HANDLER_REQ(hdl);
	cherokee_buffer_t  *tmp  = THREAD_TMP_BUF1(HANDLER_THREAD(hdl));

	/* Append the query string
	 */
	if ((req->web_directory.len > 1) &&
	    (req->options & conn_op_document_root))
	{
		cherokee_buffer_prepend_buf (&req->request, &req->web_directory);
	}

	if  (! cherokee_buffer_is_empty (&req->query_string)) {
		cherokee_buffer_add_str (&req->request, "?");
		cherokee_buffer_add_buffer (&req->request, &req->query_string);
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
		if (req->web_directory.len == 1)
			subject = req->request.buf + (req->web_directory.len - 1);
		else
			subject = req->request.buf + req->web_directory.len;

		subject_len = strlen (subject);

		/* Case 1: No conn substitution, No local regex
		 */
		if ((list->re == NULL) &&
		    (req->regex_ovecsize == 0))
		{
			TRACE (ENTRIES, "Using req->ovector, size=%d\n",
			       req->regex_ovecsize);
		}

		/* Case 2: Cached conn substitution
		 */
		else if (list->re == NULL) {
			memcpy (ovector,
				req->regex_ovector,
				OVECTOR_LEN * sizeof(cint_t));

			rc = req->regex_ovecsize;
			TRACE (ENTRIES, "Using req->ovector, size=%d\n", rc);
		}

		/* Case 3: Use the rule-subentry regex
		 */
		else {
			rc = pcre_exec (list->re, NULL, subject, subject_len, 0, 0, ovector, OVECTOR_LEN);
			if (rc == 0) {
				LOG_ERROR_S (CHEROKEE_ERROR_HANDLER_REGEX_GROUPS);
			}

			TRACE (ENTRIES, "subject = \"%s\" + len(\"%s\")-1=%d\n",
			       req->request.buf, req->web_directory.buf, req->web_directory.len - 1);
			TRACE (ENTRIES, "pcre_exec: subject=\"%s\" -> %d\n", subject, rc);

			if (rc <= 0) {
				continue;
			}
		}

		/* Make a copy of the original request before rewrite it
		 */
		if (cherokee_buffer_is_empty (&req->request_original)) {
			cherokee_buffer_add_buffer (&req->request_original, &req->request);
		}

		cherokee_buffer_clean (tmp);
		cherokee_buffer_add (tmp, subject, subject_len);

		/* Internal redirect
		 */
		if (list->hidden == true) {
			int   len;
			char *args;

			cherokee_buffer_clean (&req->request);
			cherokee_buffer_clean (&req->pathinfo);
			cherokee_buffer_clean (&req->web_directory);
			cherokee_buffer_clean (&req->local_directory);

			cherokee_buffer_ensure_size (&req->request, req->request.len + subject_len);
			substitute (hdl,
				    &list->subs,    /* regex str */
				    tmp,            /* source    */
				    &req->request, /* target    */
				    ovector, rc);   /* ovector   */


			/* Arguments */
			cherokee_split_arguments (&req->request, 0, &args, &len);
			if (len > 0) {
				cherokee_buffer_clean (&req->query_string);
				cherokee_buffer_add (&req->query_string, args, len);
				cherokee_buffer_drop_ending (&req->request, len+1);
			}

			/* Non-global redirection */
			if (req->request.buf[0] != '/') {
				cherokee_buffer_prepend_str (&req->request, "/");
			}

			TRACE (ENTRIES, "Hidden redirect to: request=\"%s\" query_string=\"%s\"\n",
			       req->request.buf, req->query_string.buf);

			return ret_eagain;
		}

		/* External redirect
		 */
		cherokee_buffer_ensure_size (&req->redirect, req->request.len + subject_len);

		substitute (hdl,
			    &list->subs,     /* regex str */
			    tmp,             /* source    */
			    &req->redirect, /* target    */
			    ovector, rc);    /* ovector   */

		TRACE (ENTRIES, "Redirect %s -> %s\n", req->request_original.buf, req->redirect.buf);

		ret = ret_ok;
		goto out;
	}

	ret = ret_ok;

out:
	if (! cherokee_buffer_is_empty (&req->query_string)) {
		cherokee_buffer_drop_ending (&req->request, req->query_string.len + 1);
	}

	if ((req->web_directory.len > 1) &&
	    (req->options & conn_op_document_root))
	{
		cherokee_buffer_move_to_begin (&req->request, req->web_directory.len);
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
	if (! cherokee_buffer_is_empty (&REQ(cnt)->redirect)) {
		TRACE (ENTRIES, "Explicit redirection to '%s'\n", REQ(cnt)->redirect.buf);
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
	int                 request_end;
	char               *request_ending;
	cherokee_request_t *req             = HANDLER_REQ(n);

	/* Maybe ::new -> match_and_substitute() has already set
	 * this redirection
	 */
	if (! cherokee_buffer_is_empty (&req->redirect)) {
		req->error_code = http_moved_permanently;
		return ret_error;
	}

	/* Check if it has the URL
	 */
	if (HDL_REDIR_PROPS(n)->url.len <= 0) {
		req->error_code = http_internal_error;
		return ret_error;
	}

	/* Try with URL directive
	 */
	request_end = (req->request.len - req->web_directory.len);
	request_ending = req->request.buf + req->web_directory.len;

	cherokee_buffer_ensure_size (&req->redirect, request_end + HDL_REDIR_PROPS(n)->url.len +1);
	cherokee_buffer_add_buffer (&req->redirect, &HDL_REDIR_PROPS(n)->url);
	cherokee_buffer_add (&req->redirect, request_ending, request_end);

	req->error_code = http_moved_permanently;
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
