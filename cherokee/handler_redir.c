/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Ayose Cazorla León <setepo@gulic.org>
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
#include "handler_redir.h"

#include "connection.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "pcre/pcre.h"
#include "regex.h"
#include "util.h"
#include "list_ext.h"

#define ENTRIES "handler,redir"


#ifndef CHEROKEE_EMBEDDED

struct cre_list {
	list_t             item;
	pcre              *re;
	char               hidden;
	cherokee_buffer_t  subs;
};


static void 
substitute_groups (cherokee_buffer_t* url, const char* subject, 
		   cherokee_buffer_t* subs, int ovector[], int stringcount)
{
	char               *s;
	cherokee_boolean_t dollar;
	cherokee_buffer_t  buff = CHEROKEE_BUF_INIT;
	
	dollar = false;
	for(s = subs->buf; *s != '\0'; s++) {

		if (dollar) {
			char num = *s - '0';

			if (num >= 0 && num <= 9) {
				cherokee_buffer_ensure_size (&buff, 1024);

				pcre_copy_substring (subject, ovector, stringcount, num, buff.buf, buff.size-1);
				cherokee_buffer_add (url, buff.buf, strlen(buff.buf));

			} else {
				/* If it is not a number, add both characters 
				 */
				cherokee_buffer_add_str (url, "$");
				cherokee_buffer_add (url, (char *)s, 1);
			}

			dollar = false;
		} else {
			if (*s == '$')
				dollar = true;
			else 
				cherokee_buffer_add (url, (char *)s, 1);
		}
	}

	cherokee_buffer_mrproper (&buff);
}


static ret_t
match_and_substitute (cherokee_handler_redir_t *n) 
{
	list_t                *i;
	cherokee_connection_t *conn = HANDLER_CONN(n);
	
	list_for_each (i, n->regex_list_ref) {
		int   ovector[OVECTOR_LEN], rc;
		char *subject; 
		int   subject_len;
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

		/* It might be matched previosly in the request parsing..
		 */
		if (list->re == NULL) {
			memcpy (ovector, conn->req_matched_ref->ovector, OVECTOR_LEN * sizeof(int));
			rc = conn->req_matched_ref->ovecsize;

		} else {
			rc = pcre_exec (list->re, NULL, subject, subject_len, 0, 0, ovector, 30);
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

			cherokee_buffer_ensure_size (&conn->request, conn->request.len + subject_len);
			cherokee_buffer_clean (&conn->request);

			substitute_groups (&conn->request, subject_copy, &list->subs, ovector, rc);

			cherokee_split_arguments (&conn->request, 0, &args, &len);

			if (len > 0) {
				cherokee_buffer_clean (&conn->query_string);
				cherokee_buffer_add (&conn->query_string, args, len);
				cherokee_buffer_drop_endding (&conn->request, len+1);
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

		return ret_ok;
	}

	return ret_ok;
}

#endif

ret_t 
cherokee_handler_redir_new (cherokee_handler_t **hdl, void *cnt, cherokee_table_t *properties)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_redir);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt);
	
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_redir_init;
	MODULE(n)->free         = (handler_func_free_t) cherokee_handler_redir_free;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_redir_add_headers;
	
	HANDLER(n)->connection  = cnt;
	HANDLER(n)->support     = hsupport_nothing;

	n->regex_list_ref       = NULL;
	n->target_url           = NULL;
	n->target_url_len       = 0;
	n->use_previous_match   = false;

	/* It needs at least the "URL" configuration parameter..
	 */
	if (cherokee_buffer_is_empty (&CONN(cnt)->redirect) && properties) {
		/* Get the URL property, if needed
		 */
		cherokee_typed_table_get_str (properties, "url", &n->target_url);
		n->target_url_len = (n->target_url == NULL ? 0 : strlen(n->target_url));

		TRACE (ENTRIES, "URL: %s\n", n->target_url);
	}

	/* Manage the regex rules
	 */
#ifndef CHEROKEE_EMBEDDED
	if (properties != NULL) {
		cherokee_typed_table_get_list (properties, "regex_list", &n->regex_list_ref);
	}

	if (n->regex_list_ref != NULL) {
		ret = match_and_substitute (n);
		if (ret == ret_eagain) {
			cherokee_handler_redir_free (n);
			return ret_eagain;
		}
	}
#endif
	
	/* Return the new handler obj
	 */
	*hdl = HANDLER(n);
	return ret_ok;	   
}


ret_t 
cherokee_handler_redir_free (cherokee_handler_redir_t *rehdl)
{
	return ret_ok;
}


ret_t 
cherokee_handler_redir_init (cherokee_handler_redir_t *n)
{
	int                    request_end;
	char                  *request_endding;
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
	if (n->target_url_len <= 0) {
		conn->error_code = http_internal_error;
		return ret_error;		
	}

	/* Try with URL directive
	 */
	request_end = (conn->request.len - conn->web_directory.len);
	request_endding = conn->request.buf + conn->web_directory.len;
	
	cherokee_buffer_ensure_size (&conn->redirect, request_end + n->target_url_len +1);
	cherokee_buffer_add (&conn->redirect, n->target_url, n->target_url_len);
	cherokee_buffer_add (&conn->redirect, request_endding, request_end);

	conn->error_code = http_moved_permanently;
	return ret_ok;
}


ret_t 
cherokee_handler_redir_add_headers (cherokee_handler_redir_t *rehdl, cherokee_buffer_t *buffer)
{
	return ret_ok;
}


static void
cre_list_free (void *data)
{
}

static ret_t
configure_rewrite (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_table_t *props)
{
	ret_t              ret;
	cint_t             hidden;
	list_t             nlist;
	struct cre_list   *n;
	cherokee_buffer_t *substring;
	pcre              *re    = NULL;
	cherokee_buffer_t *regex = NULL;
	list_t            *plist = NULL;

	TRACE(ENTRIES, "Converting rewrite rule '%s'\n", conf->key.buf);

	INIT_LIST_HEAD (&nlist);

	/* Query conf
	 */
	cherokee_config_node_read_int (conf, "show", &hidden);
	hidden = !hidden;

	ret = cherokee_config_node_read (conf, "regex", &regex);
	if (ret == ret_ok) {
		ret = cherokee_regex_table_get (srv->regexs, regex->buf, (void **)&re);
		if (ret != ret_ok) return ret;
	}

	ret = cherokee_config_node_read (conf, "substring", &substring);
	if (ret != ret_ok) return ret;
	
	/* New RegEx
	 */
	n = (struct cre_list*)malloc(sizeof(struct cre_list));

	INIT_LIST_HEAD (&n->item);
	n->re     = re;
	n->hidden = hidden;
	
	cherokee_buffer_init (&n->subs);
	cherokee_buffer_add_buffer (&n->subs, substring);

	/* Add the list
	 */
	cherokee_typed_table_get_list (props, "regex_list", &plist);
	
	if (plist == NULL) {
		list_add_tail ((list_t *)n, &nlist);
		cherokee_typed_table_add_list (props, "regex_list", &nlist, cre_list_free);
	} else {
		list_add_tail ((list_t *)n, plist);
	}

	return ret_ok;
}


static ret_t 
cherokee_handler_redir_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_table_t **props)
{
	ret_t   ret;
	list_t *i, *j;

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		ret = cherokee_typed_table_instance (props);
		if (ret != ret_ok) return ret;

		if (equal_buf_str (&subconf->key, "url")) {
			ret = cherokee_typed_table_add_str (*props, "url", strdup(subconf->val.buf));
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "rewrite")) {
			cherokee_config_node_foreach (j, subconf) {
				ret = configure_rewrite (CONFIG_NODE(j), srv, *props);
				if (ret != ret_ok) return ret;
			}
		}
	}
	
	return ret_ok;
}


/* Library init function
 */
void 
MODULE_INIT(redir) (cherokee_module_loader_t *loader)
{
}


cherokee_module_info_handler_t MODULE_INFO(redir) = {
	.module.type      = cherokee_handler,                  /* type         */
	.module.new_func  = cherokee_handler_redir_new,        /* new func     */
	.module.configure = cherokee_handler_redir_configure,  /* configure    */
	.valid_methods    = http_all_methods                   /* http methods */
};


