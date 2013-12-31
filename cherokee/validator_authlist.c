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
#include "validator_authlist.h"

#include "connection.h"
#include "connection-protected.h"
#include "plugin_loader.h"
#include "util.h"

#define ENTRIES "validator,authlist"


/* Plug-in initialization
 */
PLUGIN_INFO_VALIDATOR_EASIEST_INIT (authlist, http_auth_basic | http_auth_digest);

typedef struct {
	cherokee_list_t   listed;
	cherokee_buffer_t user;
	cherokee_buffer_t password;
} entry_t;

#define ENTRY(e) ((entry_t *)(e))


static ret_t
entry_new (entry_t **ret)
{
	entry_t *entry;
	entry = (entry_t *) malloc(sizeof(entry_t));
	if (entry == NULL)
		return ret_nomem;

	INIT_LIST_HEAD (&entry->listed);
	cherokee_buffer_init (&entry->user);
	cherokee_buffer_init (&entry->password);

	*ret = entry;
	return ret_ok;
}

static ret_t
entry_free (entry_t *entry)
{
	cherokee_buffer_mrproper (&entry->user);
	cherokee_buffer_mrproper (&entry->password);

	free (entry);
	return ret_ok;
}


static ret_t
props_free (cherokee_validator_authlist_props_t *props)
{
	cherokee_list_t *i, *j;

	list_for_each_safe (i, j, &props->users) {
		entry_free (ENTRY(i));
	}

	return cherokee_validator_props_free_base (VALIDATOR_PROPS(props));
}


static ret_t
add_new_user (cherokee_validator_authlist_props_t *props,
              cherokee_config_node_t              *conf)
{
	ret_t              ret;
	cherokee_buffer_t *tmp;
	entry_t           *entry = NULL;

	ret = entry_new (&entry);
	if (ret != ret_ok)
		return ret;

	ret = cherokee_config_node_read (conf, "user", &tmp);
	if (ret != ret_ok) {
		LOG_ERROR (CHEROKEE_ERROR_VALIDATOR_AUTHLIST_USER, conf->val.buf);
		entry_free (entry);
		return ret_error;
	}
	cherokee_buffer_add_buffer (&entry->user, tmp);

	TRACE (ENTRIES, "Adding user='%s'\n", entry->user.buf);

	ret = cherokee_config_node_read (conf, "password", &tmp);
	if (ret != ret_ok) {
		LOG_ERROR (CHEROKEE_ERROR_VALIDATOR_AUTHLIST_PASSWORD, conf->val.buf);
		entry_free (entry);
		return ret_error;
	}
	cherokee_buffer_add_buffer (&entry->password, tmp);

	cherokee_list_add (&entry->listed, &props->users);
	return ret_ok;
}

ret_t
cherokee_validator_authlist_configure (cherokee_config_node_t   *conf,
                                       cherokee_server_t        *srv,
                                       cherokee_module_props_t **_props)
{
	ret_t                                ret;
	cherokee_list_t                     *i;
	cherokee_validator_authlist_props_t *props;
	cherokee_config_node_t              *subconf;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, validator_authlist_props);
		cherokee_validator_props_init_base (VALIDATOR_PROPS(n), MODULE_PROPS_FREE(props_free));

		INIT_LIST_HEAD (&n->users);
		*_props = MODULE_PROPS(n);
	}

	props = PROP_AUTHLIST(*_props);

	/* Read the configuration tree
	 */
	ret = cherokee_config_node_get (conf, "list", &subconf);
	if (ret != ret_ok) {
		LOG_WARNING_S (CHEROKEE_ERROR_VALIDATOR_AUTHLIST_EMPTY);
		goto out;
	}

	cherokee_config_node_foreach (i, subconf) {
		ret = add_new_user (props, CONFIG_NODE(i));
		if (ret != ret_ok)
			return ret;
	}

out:
	return ret_ok;
}


static ret_t
authlist_free (cherokee_validator_authlist_t *authlist)
{
	cherokee_validator_free_base (VALIDATOR(authlist));
	return ret_ok;
}


static ret_t
authlist_check (cherokee_validator_authlist_t *authlist,
                cherokee_connection_t         *conn)
{
	int                                  re;
	ret_t                                ret;
	entry_t                             *entry;
	cherokee_list_t                     *i;
	cherokee_validator_authlist_props_t *props = VAL_AUTHLIST_PROP(authlist);

	list_for_each (i, &props->users) {
		entry = ENTRY(i);

		/* Check the user name
		 */
		re = cherokee_buffer_cmp_buf (&entry->user, &conn->validator->user);
		if (re != 0)
			continue;

		/* Check his password
		 */
		switch (conn->req_auth_type) {
		case http_auth_basic:
			/* The password field is empty
			 */
			if ((cherokee_buffer_is_empty (&entry->password)) &&
			    (cherokee_buffer_is_empty (&conn->validator->passwd)))
				return ret_ok;

			/* Check the real password
			 */
			re = cherokee_buffer_cmp_buf (&entry->password,
			                              &conn->validator->passwd);
			if (re == 0)
				return ret_ok;
			break;

		case http_auth_digest:
			ret = cherokee_validator_digest_check (VALIDATOR(authlist),
			                                       &entry->password, conn);
			if (ret == ret_ok)
				return ret;
			break;

		default:
			SHOULDNT_HAPPEN;
		}
	}

	return ret_deny;
}


static ret_t
authlist_add_headers (cherokee_validator_authlist_t *authlist,
                      cherokee_connection_t         *conn,
                      cherokee_buffer_t             *buf)
{
	UNUSED(authlist);
	UNUSED(conn);
	UNUSED(buf);

	return ret_ok;
}


ret_t
cherokee_validator_authlist_new (cherokee_validator_authlist_t **authlist,
                                 cherokee_module_props_t        *props)
{
	CHEROKEE_NEW_STRUCT(n,validator_authlist);

	/* Init
	 */
	cherokee_validator_init_base (VALIDATOR(n), VALIDATOR_PROPS(props), PLUGIN_INFO_VALIDATOR_PTR(authlist));
	VALIDATOR(n)->support = http_auth_basic | http_auth_digest;

	MODULE(n)->free           = (module_func_free_t)           authlist_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       authlist_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) authlist_add_headers;

	/* Return obj
	 */
	*authlist = n;
	return ret_ok;
}
