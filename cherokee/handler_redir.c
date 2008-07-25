/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Ayose Cazorla León <setepo@gulic.org>
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
struct cre_list {
	cherokee_list_t    item;
	pcre              *re;
	char               hidden;
	cherokee_buffer_t  subs;
};


static void 
substitute_groups (cherokee_buffer_t *url, const char *subject, 
		   cherokee_buffer_t *subs, int ovector[], int stringcount)
{
	cint_t              re;
	char                num;
	cherokee_boolean_t  dollar;
	char               *s         = subs->buf;
	const char         *substring = NULL;

	for (dollar = false; *s != '\0'; s++) {
		if (! dollar) {
			if (*s == '$')
				dollar = true;
			else 
				cherokee_buffer_add (url, (const char *)s, 1);
			continue;
		}

		num = *s - '0';
		if (num >= 0 && num <= 9) {
			re = pcre_get_substring (subject, ovector, stringcount, num, &substring);
			if ((re < 0) || (substring == NULL)) {
				dollar = false;
				continue;
			}
			cherokee_buffer_add (url, substring, strlen(substring));
			pcre_free_substring (substring);

		} else {
			/* If it is not a number, add both characters 
			 */
			cherokee_buffer_add_str (url, "$");
			cherokee_buffer_add (url, (const char *)s, 1);
		}
		
		dollar = false;
	}
}


static ret_t
match_and_substitute (cherokee_handler_redir_t *n) 
{
	cherokee_list_t       *i;
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(n);
	
	/* Append the query string
	 */
	if (! cherokee_buffer_is_empty (&conn->query_string)) {
		cherokee_buffer_add_str (&conn->request, "?");
		cherokee_buffer_add_buffer (&conn->request, &conn->query_string);
	}
	
	/* Try to match it
	 */
	list_for_each (i, &HDL_REDIR_PROPS(n)->regex_list) {
		cint_t           rc = 0;
		char            *subject; 
		cint_t           subject_len;
		cint_t           ovector[OVECTOR_LEN];
		struct cre_list *list = (struct cre_list *)i;

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
				PRINT_ERROR_S("Too many groups in the regex\n");
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
		cherokee_buffer_add_buffer (&conn->request_original, &conn->request);

		/* Internal redirect
		 */
		if (list->hidden == true) {
			char *args;
			int   len;
			char *subject_copy = strdup (subject);

			cherokee_buffer_clean (&conn->pathinfo);
			cherokee_buffer_clean (&conn->request);

			cherokee_buffer_ensure_size (&conn->request, conn->request.len + subject_len);
			substitute_groups (&conn->request, subject_copy, &list->subs, ovector, rc);

			cherokee_split_arguments (&conn->request, 0, &args, &len);

			if (len > 0) {
				cherokee_buffer_clean (&conn->query_string);
				cherokee_buffer_add (&conn->query_string, args, len);
				cherokee_buffer_drop_ending (&conn->request, len+1);
			}

			TRACE (ENTRIES, "Hidden redirect to: request=\"%s\" query_string=\"%s\"\n", 
			       conn->request.buf, conn->query_string.buf);
			
			free (subject_copy);
			return ret_eagain;
		}
		
		/* External redirect
		 */
		cherokee_buffer_ensure_size (&conn->redirect, conn->request.len + subject_len);
		substitute_groups (&conn->redirect, subject, &list->subs, ovector, rc);

		TRACE (ENTRIES, "Redirect %s -> %s\n", conn->request_original.buf, conn->redirect.buf);

		ret = ret_ok;
		goto out;
	}

	ret = ret_ok;

out:
	if (! cherokee_buffer_is_empty (&conn->query_string))
		cherokee_buffer_drop_ending (&conn->request, conn->query_string.len + 1);

	return ret;
}

static ret_t
configure_rewrite (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_handler_redir_props_t *props)
{
	ret_t               ret;
	cint_t              hidden;
	struct cre_list    *n;
	cherokee_buffer_t  *substring;
	pcre               *re         = NULL;
	cherokee_buffer_t  *regex      = NULL;

	TRACE(ENTRIES, "Converting rewrite rule '%s'\n", conf->key.buf);

	/* Query conf
	 */
	cherokee_config_node_read_int (conf, "show", &hidden);
	hidden = !hidden;

	ret = cherokee_config_node_read (conf, "regex", &regex);
	if (ret == ret_ok) {
		ret = cherokee_regex_table_get (srv->regexs, regex->buf, (void **)&re);
		if (ret != ret_ok)
			return ret;
	} 

	ret = cherokee_config_node_read (conf, "substring", &substring);
	if (ret != ret_ok)
		return ret;

	/* New RegEx
	 */
	n = (struct cre_list*)malloc(sizeof(struct cre_list));

	INIT_LIST_HEAD (&n->item);
	n->re         = re;
	n->hidden     = hidden;
	
	cherokee_buffer_init (&n->subs);
	cherokee_buffer_add_buffer (&n->subs, substring);

	/* Add the list
	 */
	cherokee_list_add_tail (LIST(n), &props->regex_list);

	return ret_ok;
}


static void
cre_entry_free (struct cre_list *n) 
{
	cherokee_buffer_mrproper (&n->subs);
	free (n);
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
	cherokee_list_t *i, *tmp;

	cherokee_buffer_mrproper (&props->url);

	list_for_each_safe (i, tmp, &props->regex_list) {
		cre_entry_free ((struct cre_list *)i);
	}

	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


ret_t 
cherokee_handler_redir_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                           ret;
	cherokee_list_t                *i, *j;
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
			cherokee_config_node_foreach (j, subconf) {
				ret = configure_rewrite (CONFIG_NODE(j), srv, PROP_REDIR(props));
				if (ret != ret_ok) return ret;
			}
		}
	}
	
	return ret_ok;
}

