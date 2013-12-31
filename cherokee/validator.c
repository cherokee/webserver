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
#include "validator.h"

#include "connection.h"
#include "connection-protected.h"
#include "header-protected.h"
#include "config_entry.h"
#include "util.h"


#define ENTRIES "validator"
#define REALM_DEFAULT "Protected"

ret_t
cherokee_validator_init_base (cherokee_validator_t             *validator,
                              cherokee_validator_props_t       *props,
                              cherokee_plugin_info_validator_t *info)
{
	cherokee_module_init_base (MODULE(validator), MODULE_PROPS(props), PLUGIN_INFO(info));

	validator->check = NULL;

	cherokee_buffer_init (&validator->user);
	cherokee_buffer_init (&validator->passwd);
	cherokee_buffer_init (&validator->realm);
	cherokee_buffer_init (&validator->response);
	cherokee_buffer_init (&validator->uri);
	cherokee_buffer_init (&validator->qop);
	cherokee_buffer_init (&validator->nonce);
	cherokee_buffer_init (&validator->cnonce);
	cherokee_buffer_init (&validator->algorithm);
	cherokee_buffer_init (&validator->nc);

	return ret_ok;
}


ret_t
cherokee_validator_free_base (cherokee_validator_t *validator)
{
	cherokee_buffer_mrproper (&validator->user);
	cherokee_buffer_mrproper (&validator->passwd);
	cherokee_buffer_mrproper (&validator->realm);
	cherokee_buffer_mrproper (&validator->response);
	cherokee_buffer_mrproper (&validator->uri);
	cherokee_buffer_mrproper (&validator->qop);
	cherokee_buffer_mrproper (&validator->nonce);
	cherokee_buffer_mrproper (&validator->cnonce);
	cherokee_buffer_mrproper (&validator->algorithm);
	cherokee_buffer_mrproper (&validator->nc);

	free (validator);
	return ret_ok;
}


ret_t
cherokee_validator_free (cherokee_validator_t *validator)
{
	module_func_free_t func;

	return_if_fail (validator!=NULL, ret_error);

	func = (module_func_free_t) MODULE(validator)->free;

	if (func == NULL) {
		return ret_error;
	}

	return func (validator);
}


ret_t
cherokee_validator_check (cherokee_validator_t *validator, void *conn)
{
	if (validator->check == NULL) {
		return ret_error;
	}

	return validator->check (validator, CONN(conn));
}


ret_t
cherokee_validator_add_headers (cherokee_validator_t *validator,
                                void                 *conn,
                                cherokee_buffer_t    *buf)
{
	if (validator->add_headers == NULL) {
		return ret_error;
	}

	return validator->add_headers (validator, CONN(conn), buf);
}


ret_t
cherokee_validator_parse_basic (cherokee_validator_t *validator, char *str, cuint_t str_len)
{
	char              *colon;
	cherokee_buffer_t  auth = CHEROKEE_BUF_INIT;

	/* Decode base64
	 */
	cherokee_buffer_add (&auth, str, str_len);
	cherokee_buffer_decode_base64 (&auth);

	/* Look for the user:passwd structure
	 */
	colon = strchr (auth.buf, ':');
	if (colon == NULL)
		goto error;

	/* Copy user and password
	 */
	cherokee_buffer_add (&validator->user, auth.buf, colon - auth.buf);
	cherokee_buffer_add (&validator->passwd, colon+1, auth.len  - ((colon+1) - auth.buf));

	TRACE (ENTRIES, "Parse basic auth got user=%s, passwd=%s\n", validator->user.buf, validator->passwd.buf);

	/* Clean up and exit
	 */
	cherokee_buffer_mrproper (&auth);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&auth);
	return ret_error;
}


ret_t
cherokee_validator_parse_digest (cherokee_validator_t *validator,
                                 char *str, cuint_t str_len)
{
	cuint_t             len;
	char               *end;
	char               *entry;
	char               *comma;
	char               *equal;
	cherokee_buffer_t   auth = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  *entry_buf;

	/* Copy authentication string
	 */
	cherokee_buffer_add (&auth, str, str_len);

	entry = auth.buf;
	end   = auth.buf + auth.len;

	do {
		/* Skip some chars
		 */
		while ((*entry == CHR_SP) ||
		       (*entry == CHR_CR) ||
		       (*entry == CHR_LF)) entry++;

		/* Check for the end
		 */
		if (entry >= end)
			break;

		comma = strchr(entry, ',');

		if (equal_str (entry, "nc")) {
			entry_buf = &validator->nc;
		} else if (equal_str (entry, "uri")) {
			entry_buf = &validator->uri;
		} else if (equal_str (entry, "qop")) {
			entry_buf = &validator->qop;
		} else if (equal_str (entry, "realm")) {
			entry_buf = &validator->realm;
		} else if (equal_str (entry, "nonce")) {
			entry_buf = &validator->nonce;
		} else if (equal_str (entry, "cnonce")) {
			entry_buf = &validator->cnonce;
		} else if (equal_str (entry, "username")) {
			entry_buf = &validator->user;
		} else if (equal_str (entry, "response")) {
			entry_buf = &validator->response;
		} else if (equal_str (entry, "algorithm")) {
			entry_buf = &validator->algorithm;
		} else {
			entry = comma + 1;
			continue;
		}

		/* Split the string [1]
		 */
		if (comma) *comma = '\0';

		equal = strchr (entry, '=');
		if (equal == NULL) {
			if (comma) *comma = ',';  /* [1] */
			continue;
		}

		/* Check for "s and skip it
		 */
		equal += (equal[1] == '"') ? 2 : 1;
		len = strlen(equal);
		while ((len > 0) &&
		       ((equal[len-1] == '"')  ||
		       (equal[len-1] == CHR_CR) ||
		       (equal[len-1] == CHR_LF))) {

			len--;
		}

		/* Copy the entry value
		 */
		cherokee_buffer_add (entry_buf, equal, len);

		/* Resore [1], and prepare next loop
		 */
		if (comma)
			*comma = ',';
		entry = comma + 1;

	} while (comma != NULL);

#if 0
	printf ("nc           %s\n", validator->nc.buf);
	printf ("uri          %s\n", validator->uri.buf);
	printf ("qop          %s\n", validator->qop.buf);
	printf ("realm        %s\n", validator->realm.buf);
	printf ("nonce        %s\n", validator->nonce.buf);
	printf ("cnonce       %s\n", validator->cnonce.buf);
	printf ("username     %s\n", validator->user.buf);
	printf ("reponse      %s\n", validator->response.buf);
	printf ("algorithm    %s\n", validator->algorithm.buf);
#endif

	/* Clean up and exit
	 */
	cherokee_buffer_mrproper (&auth);
	return ret_ok;
}


static ret_t
digest_HA2 (cherokee_validator_t *validator, cherokee_buffer_t *buf, cherokee_connection_t *conn)
{
	ret_t       ret;
	const char *method;
	cuint_t     method_len;

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&validator->uri))
		return ret_deny;

	ret = cherokee_http_method_to_string (conn->header.method, &method, &method_len);
	if (unlikely (ret != ret_ok))
		return ret;

	cherokee_buffer_ensure_size (buf, method_len + 1 + validator->uri.len);

	cherokee_buffer_add        (buf, method, method_len);
	cherokee_buffer_add_str    (buf, ":");
	cherokee_buffer_add_buffer (buf, &validator->uri);

	cherokee_buffer_encode_md5_digest (buf);

	return ret_ok;
}


ret_t
cherokee_validator_digest_response (cherokee_validator_t  *validator,
                                    char                  *A1,
                                    cherokee_buffer_t     *buf,
                                    cherokee_connection_t *conn)
{
	ret_t             ret;
	cherokee_buffer_t a2   = CHEROKEE_BUF_INIT;

	/* A1 has to be in string of length 32:
	 * MD5_digest(user":"realm":"passwd)
	 */

	/* Sanity checks
	 */
	if (A1 == NULL)
		return ret_deny;

	if (cherokee_buffer_is_empty (&validator->nonce))
		return ret_deny;

	/* Build A2
	 */
	ret = digest_HA2 (validator, &a2, conn);
	if (ret != ret_ok)
		goto error;

	/* Build the final string
	 */
	cherokee_buffer_ensure_size (buf, 32 + a2.len + validator->nonce.len + 4);

	cherokee_buffer_add (buf, A1, 32);
	cherokee_buffer_add_str (buf, ":");
	cherokee_buffer_add_buffer (buf, &validator->nonce);
	cherokee_buffer_add_str (buf, ":");

	if (!cherokee_buffer_is_empty (&validator->qop)) {
		if (!cherokee_buffer_is_empty (&validator->nc))
			cherokee_buffer_add_buffer (buf, &validator->nc);
		cherokee_buffer_add_str (buf, ":");
		if (!cherokee_buffer_is_empty (&validator->cnonce))
			cherokee_buffer_add_buffer (buf, &validator->cnonce);
		cherokee_buffer_add_str (buf, ":");
		cherokee_buffer_add_buffer (buf, &validator->qop);
		cherokee_buffer_add_str (buf, ":");
	}

	cherokee_buffer_add_buffer (buf, &a2);
	cherokee_buffer_encode_md5_digest (buf);
	cherokee_buffer_mrproper (&a2);

	return ret_ok;

error:
	cherokee_buffer_mrproper (&a2);
	return ret;
}


ret_t
cherokee_validator_digest_check (cherokee_validator_t *validator, cherokee_buffer_t *passwd, cherokee_connection_t *conn)
{
	ret_t             ret;
	int               re   = -1;
	cherokee_buffer_t a1   = CHEROKEE_BUF_INIT;
	cherokee_buffer_t buf  = CHEROKEE_BUF_INIT;

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&validator->user) ||
	    cherokee_buffer_is_empty (&validator->realm))
		return ret_deny;

	/* Build A1
	 */
	cherokee_buffer_ensure_size (&a1,
	                             validator->user.len  + 1 +
	                             validator->realm.len + 1 +
	                             passwd->len);

	cherokee_buffer_add_buffer (&a1, &validator->user);
	cherokee_buffer_add_str    (&a1, ":");
	cherokee_buffer_add_buffer (&a1, &validator->realm);
	cherokee_buffer_add_str    (&a1, ":");
	cherokee_buffer_add_buffer (&a1, passwd);

	cherokee_buffer_encode_md5_digest (&a1);

	/* Build a possible response
	 */
	ret = cherokee_validator_digest_response (validator, a1.buf, &buf, conn);
	if (unlikely(ret != ret_ok))
		goto go_out;

	/* Compare and return
	 */
	re = cherokee_buffer_cmp_buf (&conn->validator->response, &buf);

go_out:
	cherokee_buffer_mrproper (&a1);
	cherokee_buffer_mrproper (&buf);

	return (re == 0) ? ret_ok : ret_deny;
}


static ret_t
add_method (char *method, void *data)
{
	cherokee_config_entry_t *entry = CONF_ENTRY(data);

	if (equal_str (method, "basic"))
		entry->authentication |= http_auth_basic;
	else if (equal_str (method, "digest"))
		entry->authentication |= http_auth_digest;
	else {
		LOG_ERROR (CHEROKEE_ERROR_VALIDATOR_METHOD_UNKNOWN, method);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
add_user  (char *val, void *data)
{
	return cherokee_avl_add_ptr (AVL(data), val, NULL);
}


ret_t
cherokee_validator_configure (cherokee_config_node_t *conf, void *config_entry)
{
	ret_t                    ret;
	cherokee_list_t         *i;
	cherokee_config_node_t  *subconf;
	cherokee_config_entry_t *entry = CONF_ENTRY(config_entry);

	cherokee_config_node_foreach (i, conf) {
		subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "realm")) {
			TRACE(ENTRIES, "Realm: %s\n", subconf->val.buf);

			ret = cherokee_buffer_dup (&subconf->val, &entry->auth_realm);
			if (ret != ret_ok)
				return ret;

		} else if (equal_buf_str (&subconf->key, "methods")) {
			TRACE(ENTRIES, "Methods: %s\n", subconf->val.buf);

			ret = cherokee_config_node_read_list (subconf, NULL, add_method, entry);
			if (ret != ret_ok)
				return ret;

		} else if (equal_buf_str (&subconf->key, "users")) {
			if (entry->users == NULL) {
				cherokee_avl_new (&entry->users);
			}

			ret = cherokee_config_node_read_list (subconf, NULL, add_user, entry->users);
			if (ret != ret_ok)
				return ret;
		}
	}

	/* Sanity checks
	 */
	if (entry->auth_realm == NULL) {
		cherokee_buffer_new (&entry->auth_realm);
	}

	if (cherokee_buffer_is_empty (entry->auth_realm)) {
		cherokee_buffer_add_str (entry->auth_realm, REALM_DEFAULT);
		TRACE (ENTRIES, "No Realm was provided. Setting default realm: %s\n", REALM_DEFAULT);
	}

	return ret_ok;
}


/* Validator properties
 */

ret_t
cherokee_validator_props_init_base  (cherokee_validator_props_t *props, module_func_props_free_t free_func)
{
	props->valid_methods = http_auth_nothing;
	return cherokee_module_props_init_base (MODULE_PROPS(props), free_func);
}


ret_t
cherokee_validator_props_free_base  (cherokee_validator_props_t *props)
{
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}

