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
#include "handler_error_redir.h"

#include "connection.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "handler,error_handler,redir"

typedef struct {
	cherokee_list_t    entry;
	cuint_t            error;
	cherokee_buffer_t  url;
	cherokee_boolean_t show;
} error_entry_t;


static void
free_error_entry (void *p)
{
	error_entry_t *entry = (error_entry_t *)p;

	cherokee_buffer_mrproper (&entry->url);
	free (entry);
}

static ret_t
props_free (cherokee_handler_error_redir_props_t *props)
{
	cherokee_list_t *i, *j;

	list_for_each_safe (i, j, &props->errors) {
		free_error_entry (i);
	}

	if (props->error_default) {
		free_error_entry (props->error_default);
	}

	return cherokee_module_props_free_base (MODULE_PROPS(props));
}

ret_t
cherokee_handler_error_redir_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                                 ret;
	cherokee_list_t                      *i;
	cherokee_handler_error_redir_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_error_redir_props);

		cherokee_module_props_init_base (MODULE_PROPS(n),
						 MODULE_PROPS_FREE(props_free));
		INIT_LIST_HEAD (&n->errors);
		n->error_default = NULL;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_ERREDIR(*_props);

	cherokee_config_node_foreach (i, conf) {
		error_entry_t          *entry;
		cint_t                  error      = 0;
		cherokee_config_node_t *subconf    = CONFIG_NODE(i);
		cherokee_boolean_t      is_default = false;

		/* Special 'Default' case
		 */
		if (equal_buf_str (&subconf->key, "default")) {
			is_default = true;

		/* Check the error number
		 */
		} else {
			ret = cherokee_atoi (subconf->key.buf, &error);
			if (ret != ret_ok) return ret;

			if (!http_type_300 (error) &&
			    !http_type_400 (error) &&
			    !http_type_500 (error))
			{
				LOG_ERROR (CHEROKEE_ERROR_HANDLER_ERROR_REDIR_CODE, subconf->key.buf);
				continue;
			}
		}

		/* New object
		 */
		entry = (error_entry_t *) malloc (sizeof(error_entry_t));
		if (entry == NULL)
			return ret_nomem;

		if (error) {
			entry->error = error;
		}
		entry->show = false;

		INIT_LIST_HEAD (&entry->entry);
		cherokee_buffer_init (&entry->url);

		/* Read child values
		 */
		ret = cherokee_config_node_copy (subconf, "url", &entry->url);
		if (ret != ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_HANDLER_ERROR_REDIR_URL, error);
			free (entry);
			return ret_error;
		}

		cherokee_config_node_read_bool (subconf, "show", &entry->show);

		TRACE(ENTRIES, "Error %d redir to '%s', show=%d\n",
		      entry->error, entry->url.buf, entry->show);

		/* Add it to the list
		 */
		if (is_default) {
			props->error_default = entry;
		} else {
			cherokee_list_add (&entry->entry, &props->errors);
		}
	}

	return ret_ok;
}


static ret_t
do_redir_external (cherokee_handler_t     **hdl,
		   cherokee_connection_t   *conn,
		   cherokee_module_props_t *props,
		   error_entry_t           *entry)
{
	cherokee_buffer_clean (&conn->redirect);
	cherokee_buffer_add_buffer (&conn->redirect, &entry->url);

	conn->error_code = http_moved_permanently;
	return cherokee_handler_redir_new (hdl, conn, props);
}

static ret_t
do_redir_internal (cherokee_connection_t *conn,
		   error_entry_t         *entry)
{
	/* Set REDIRECT_URL
	 */
	cherokee_buffer_clean (&conn->error_internal_url);
	cherokee_buffer_add_buffer (&conn->error_internal_url, &conn->request);

	/* Set REDIRECT_QUERY_STRING
	 */
	cherokee_buffer_clean (&conn->error_internal_qs);
	cherokee_buffer_add_buffer (&conn->error_internal_qs, &conn->query_string);

	/* Clean up the connection
	 */
	cherokee_buffer_clean (&conn->pathinfo);
	cherokee_buffer_clean (&conn->request);
	cherokee_buffer_clean (&conn->local_directory);

	/* Set the new request
	 */
	cherokee_buffer_add_buffer (&conn->request, &entry->url);

	/* Store the previous error code
	 */
	conn->error_internal_code = conn->error_code;
	return ret_eagain;
}

ret_t
cherokee_handler_error_redir_new (cherokee_handler_t     **hdl,
				  cherokee_connection_t   *conn,
				  cherokee_module_props_t *props)
{
	cherokee_list_t *i;
	error_entry_t   *default_error;

	/* Error list
	 */
	list_for_each (i, &PROP_ERREDIR(props)->errors) {
		error_entry_t *entry = (error_entry_t *)i;

		if (entry->error != conn->error_code) {
			continue;
		}

		if (entry->show) {
			return do_redir_external (hdl, conn, props, entry);
		} else {
			return do_redir_internal (conn, entry);
		}
	}

	/* Default error
	 */
	default_error = (error_entry_t *) PROP_ERREDIR(props)->error_default;
	if (default_error) {
		if (default_error->show) {
			return do_redir_external (hdl, conn, props, default_error);
		} else {
			return do_redir_internal (conn, default_error);
		}
	}

	return ret_error;
}


/* Library init function
 */
static cherokee_boolean_t _error_redir_is_init = false;

void
PLUGIN_INIT_NAME(error_redir) (cherokee_plugin_loader_t *loader)
{
	/* Is init?
	 */
	if (_error_redir_is_init) return;
	_error_redir_is_init = true;

	/* Load the dependences
	 */
	cherokee_plugin_loader_load (loader, "redir");
}

PLUGIN_INFO_HANDLER_EASY_INIT (error_redir, http_all_methods);

