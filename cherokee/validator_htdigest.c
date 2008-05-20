/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
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
#include "validator_htdigest.h"
#include "plugin_loader.h"
#include "connection-protected.h"


/* Plug-in initialization
 */
PLUGIN_INFO_VALIDATOR_EASIEST_INIT (htdigest, http_auth_basic | http_auth_digest);


static ret_t 
props_free (cherokee_validator_htdigest_props_t *props)
{
	cherokee_buffer_mrproper (&props->password_file);
	return cherokee_validator_props_free_base (VALIDATOR_PROPS(props));
}


ret_t 
cherokee_validator_htdigest_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                                ret;
	cherokee_config_node_t              *subconf;
	cherokee_validator_htdigest_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, validator_htdigest_props);

		cherokee_validator_props_init_base (VALIDATOR_PROPS(n), MODULE_PROPS_FREE(props_free));
		cherokee_buffer_init (&n->password_file);
		
		*_props = MODULE_PROPS(n);
	}

	props = PROP_HTDIGEST(*_props);

	ret = cherokee_config_node_get (conf, "passwdfile", &subconf);
	if (ret == ret_ok) {
		cherokee_buffer_add_buffer (&props->password_file, &subconf->val);
	}

	return ret_ok;
}


ret_t 
cherokee_validator_htdigest_new (cherokee_validator_htdigest_t **htdigest, cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT(n,validator_htdigest);

	/* Init 	
	 */
	cherokee_validator_init_base (VALIDATOR(n), VALIDATOR_PROPS(props), PLUGIN_INFO_VALIDATOR_PTR(htdigest));
	VALIDATOR(n)->support = http_auth_basic | http_auth_digest;

	MODULE(n)->free           = (module_func_free_t)           cherokee_validator_htdigest_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       cherokee_validator_htdigest_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) cherokee_validator_htdigest_add_headers;

	/* Checks
	 */
	if (cherokee_buffer_is_empty (&VAL_HTDIGEST_PROP(n)->password_file)) {
		PRINT_MSG_S ("htdigest validator needs a password file\n");
		return ret_error;
	}

	*htdigest = n;
	return ret_ok;
}


ret_t 
cherokee_validator_htdigest_free (cherokee_validator_htdigest_t *htdigest)
{
	cherokee_validator_free_base (VALIDATOR(htdigest));
	return ret_ok;
}


static ret_t
build_HA1 (cherokee_connection_t *conn, cherokee_buffer_t *buf)
{
	cherokee_buffer_add_va (buf, "%s:%s:%s", conn->validator->user.buf, conn->realm_ref->buf, conn->validator->passwd.buf);
	cherokee_buffer_encode_md5_digest (buf);
	return ret_ok;
}


static ret_t
extract_user_entry (cherokee_buffer_t *file, char *user_, char **user, char **realm, char **passwd)
{
	char *end      = file->buf + file->len;
	char *pos      = file->buf;
	int   user_len = strlen (user_);
		
	while (pos < end) {
		char *eol;
		
		/* Limit the line
		 */
		eol = strchr (pos, '\n');
		if (eol != NULL) 
			*eol = '\0';
		
		/* Check the user
		 */
		if ((pos[user_len] == ':') && 
		    (strncmp (pos, user_, user_len) == 0)) {
			char *tmp;

			*user = pos;

			tmp = strchr(pos, ':');
			if (!tmp)
				return ret_error;
			*tmp = '\0';
			*realm = tmp + 1;

			tmp = strchr (*realm, ':');
			if (!tmp)
				return ret_error;
			*tmp = '\0';
			*passwd = tmp + 1;

			return ret_ok;
		}

		/* Look for the next line
		 */
		pos = eol;
		while ((*pos == CHR_CR) || (*pos == CHR_LF)) pos++;
	}

	return ret_not_found;
}


static ret_t
validate_basic (cherokee_validator_htdigest_t *htdigest, cherokee_connection_t *conn, cherokee_buffer_t *file)
{
	ret_t               ret;
	cherokee_boolean_t  equal;
	char               *user   = NULL;
	char               *realm  = NULL;
	char               *passwd = NULL;
	cherokee_buffer_t   ha1 = CHEROKEE_BUF_INIT;

	UNUSED(htdigest);

	/* Extact the right entry information
	 */
	ret = extract_user_entry (file, conn->validator->user.buf, &user, &realm, &passwd);
	if (ret != ret_ok)
		return ret;

	/* Build the hash
	 */
	build_HA1 (conn, &ha1);

	/* Compare it with the stored hash, clean, and return
	 */
	equal = (strncmp(ha1.buf, passwd, ha1.len) == 0);
	cherokee_buffer_mrproper (&ha1);

	return (equal) ? ret_ok : ret_not_found;
}


static ret_t
validate_digest (cherokee_validator_htdigest_t *htdigest, cherokee_connection_t *conn, cherokee_buffer_t *file)
{
	int                re;
	ret_t              ret;
	char              *user   = NULL;
	char              *realm  = NULL;
	char              *passwd = NULL;
	cherokee_buffer_t  buf    = CHEROKEE_BUF_INIT;

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&conn->validator->response)) 
		return ret_error;

	/* Extact the right entry information
	 */
	ret = extract_user_entry (file, conn->validator->user.buf, &user, &realm, &passwd);
	if (unlikely(ret != ret_ok))
		return ret;

	/* Build the hash:
	 * In this case passwd is the HA1 hash: md5(user:realm:passwd)
	 */
	ret = cherokee_validator_digest_response (VALIDATOR(htdigest), passwd, &buf, conn);
	if (unlikely(ret != ret_ok))
		goto go_out;

	/* Compare and return
	 */
	re = cherokee_buffer_cmp_buf (&conn->validator->response, &buf);

go_out:
	cherokee_buffer_mrproper (&buf);
	return (re == 0) ? ret_ok : ret_deny;
}


ret_t 
cherokee_validator_htdigest_check (cherokee_validator_htdigest_t *htdigest, cherokee_connection_t *conn)
{
	ret_t             ret;
	cherokee_buffer_t file = CHEROKEE_BUF_INIT;
	
	/* Ensure that we have all what we need
	 */
	if ((conn->validator == NULL) ||
	    cherokee_buffer_is_empty (&conn->validator->user)) 
		return ret_error;

	if (cherokee_buffer_is_empty (&VAL_HTDIGEST_PROP(htdigest)->password_file))
		return ret_error;

	/* Read the whole file
	 */
	ret = cherokee_buffer_read_file (&file, VAL_HTDIGEST_PROP(htdigest)->password_file.buf);
	if (ret != ret_ok) {
		ret = ret_error;
		goto out;
	}

	/* Authenticate
	 */
	if (conn->req_auth_type & http_auth_basic) {
		ret = validate_basic (htdigest, conn, &file);

	} else if (conn->req_auth_type & http_auth_digest) {
		ret = validate_digest (htdigest, conn, &file);

	} else {
		SHOULDNT_HAPPEN;
	}

out:
	cherokee_buffer_mrproper (&file);
	return ret;
}


ret_t 
cherokee_validator_htdigest_add_headers (cherokee_validator_htdigest_t *htdigest, cherokee_connection_t *conn, cherokee_buffer_t *buf)
{
	UNUSED(htdigest);
	UNUSED(conn);
	UNUSED(buf);

	return ret_ok;
}

