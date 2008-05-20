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

#include <errno.h>

#include "plugin_loader.h"
#include "validator_ldap.h"
#include "connection-protected.h"
#include "util.h"


#define ENTRIES "validador,ldap"
#define LDAP_DEFAULT_PORT 389

#ifndef LDAP_OPT_SUCCESS
# define LDAP_OPT_SUCCESS 0
#endif


/* Plug-in initialization
 */
PLUGIN_INFO_VALIDATOR_EASIEST_INIT (ldap, http_auth_basic);


static ret_t 
props_free (cherokee_validator_ldap_props_t *props)
{
	cherokee_buffer_mrproper (&props->server);
	cherokee_buffer_mrproper (&props->binddn);
	cherokee_buffer_mrproper (&props->bindpw);
	cherokee_buffer_mrproper (&props->basedn);
	cherokee_buffer_mrproper (&props->filter);
	cherokee_buffer_mrproper (&props->ca_file);

	return cherokee_validator_props_free_base (VALIDATOR_PROPS(props));
}


ret_t 
cherokee_validator_ldap_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	cherokee_list_t                 *i;
	cherokee_validator_ldap_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, validator_ldap_props);

		cherokee_validator_props_init_base (VALIDATOR_PROPS(n), MODULE_PROPS_FREE(props_free));

		n->port = LDAP_DEFAULT_PORT;
		n->tls  = false;

		cherokee_buffer_init (&n->server);
		cherokee_buffer_init (&n->binddn);
		cherokee_buffer_init (&n->bindpw);
		cherokee_buffer_init (&n->basedn);
		cherokee_buffer_init (&n->filter);
		cherokee_buffer_init (&n->ca_file);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_LDAP(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);
		
		if (equal_buf_str (&subconf->key, "server")) {
			cherokee_buffer_add_buffer (&props->server, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "port")) {
			props->port = atoi (subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "bind_dn")) {
			cherokee_buffer_add_buffer (&props->binddn, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "bind_pw")) {
			cherokee_buffer_add_buffer (&props->bindpw, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "base_dn")) {
			cherokee_buffer_add_buffer (&props->basedn, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "filter")) {
			cherokee_buffer_add_buffer (&props->filter, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "tls")) {
			props->tls = atoi (subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "ca_file")) {
			cherokee_buffer_add_buffer (&props->ca_file, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "methods") || 
			   equal_buf_str (&subconf->key, "realm")) {
			/* Not handled here
			 */
		} else {
			PRINT_MSG ("ERROR: Validator LDAP: Unknown key: '%s'\n", subconf->key.buf);
			return ret_error;
		}
	}

	/* Checks
	 */
	if (cherokee_buffer_is_empty (&props->basedn)) {
		PRINT_ERROR_S ("ERROR: LDAP validator: An entry 'base_dn' is needed\n");
		return ret_error;
	}
	if (cherokee_buffer_is_empty (&props->filter)) {
		PRINT_ERROR_S ("ERROR: LDAP validator: An entry 'filter' is needed\n");
		return ret_error;
	}
	if (cherokee_buffer_is_empty (&props->server)) {
		PRINT_ERROR_S ("ERROR: LDAP validator: An entry 'server' is needed\n");
		return ret_error;
	}

	if ((cherokee_buffer_is_empty (&props->bindpw) &&
	     (! cherokee_buffer_is_empty (&props->basedn)))) {
		PRINT_ERROR_S ("ERROR: LDAP validator: Potential security problem found:\n"
			       "\tanonymous bind validation. Check (RFC 2251, section 4.2.2)\n");
		return ret_error;
	}

	return ret_ok;
}


static ret_t
init_ldap_connection (cherokee_validator_ldap_t *ldap, cherokee_validator_ldap_props_t *props)
{
	int re;
	int val;

	/* Connect
	 */
	ldap->conn = ldap_init (props->server.buf, props->port);
	if (ldap->conn == NULL) {
		PRINT_ERRNO (errno, "Couldn't connect to LDAP: %s:%d: '${errno}'", 
			     props->server.buf, props->port);
		return ret_error;
	}

	TRACE (ENTRIES, "Connected to %s:%d\n", props->server.buf, props->port);

	/* Set LDAP protocol version
	 */
	val = LDAP_VERSION3;
	re = ldap_set_option (ldap->conn, LDAP_OPT_PROTOCOL_VERSION, &val);
	if (re != LDAP_OPT_SUCCESS) {
		PRINT_ERROR ("ERROR: LDAP validator: Couldn't set the LDAP version 3: %s\n", 
			     ldap_err2string (re));
		return ret_error;		
	}

	TRACE (ENTRIES, "LDAP protocol version %d set\n", LDAP_VERSION3);

	/* Secure connections
	 */
	if (props->tls) {
#ifdef LDAP_OPT_X_TLS
		if (! cherokee_buffer_is_empty (&props->ca_file)) {
			re = ldap_set_option (NULL, LDAP_OPT_X_TLS_CACERTFILE, props->ca_file.buf);
			if (re != LDAP_OPT_SUCCESS) {
				PRINT_ERROR ("ERROR: LDAP validator: Couldn't set CA file %s: %s\n", 
					     props->ca_file.buf, ldap_err2string (re));
				return ret_error; 
			}
		}
#else
		PRINT_ERROR_S ("Can't StartTLS, it isn't supported by LDAP client libraries\n");
#endif
	}

	/* Bind
	 */
	if (cherokee_buffer_is_empty (&props->binddn)) {
		TRACE (ENTRIES, "anonymous bind %s", "\n");
		re = ldap_simple_bind_s (ldap->conn, NULL, NULL);
	} else {
		TRACE (ENTRIES, "bind user=%s password=%s\n", 
		       props->binddn.buf, props->bindpw.buf);
		re = ldap_simple_bind_s (ldap->conn, props->binddn.buf, props->bindpw.buf);
	}

	if (re != LDAP_SUCCESS) {
		PRINT_ERROR ("Couldn't bind (%s:%d): %s:%s : %s\n", 
			     props->server.buf, props->port, props->binddn.buf, 
			     props->bindpw.buf, ldap_err2string (re));
		return ret_error;		
	}

	return ret_ok;
}


ret_t 
cherokee_validator_ldap_new (cherokee_validator_ldap_t **ldap, cherokee_module_props_t *props)
{	  
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n,validator_ldap);

	/* Init 		
	 */
	cherokee_validator_init_base (VALIDATOR(n), VALIDATOR_PROPS(props), PLUGIN_INFO_VALIDATOR_PTR(ldap));
	VALIDATOR(n)->support = http_auth_basic;

	MODULE(n)->free           = (module_func_free_t)           cherokee_validator_ldap_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       cherokee_validator_ldap_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) cherokee_validator_ldap_add_headers;

	/* Init properties
	 */
	cherokee_buffer_init (&n->filter);

	/* Connect to the LDAP server 
	 */
	ret = init_ldap_connection (n, PROP_LDAP(props));
	if (ret != ret_ok) {
		cherokee_validator_ldap_free (n);
		return ret;
	}

	*ldap = n;
	return ret_ok;
}

ret_t 
cherokee_validator_ldap_free (cherokee_validator_ldap_t *ldap)
{
	cherokee_buffer_mrproper (&ldap->filter);
	return ret_ok;
}


static ret_t 
validate_dn (cherokee_validator_ldap_props_t *props, char *dn, char *password)
{
	int   re;
	int   val;
	LDAP *conn;

	conn = ldap_init (props->server.buf, props->port);
	if (conn == NULL) return ret_error;

	val = LDAP_VERSION3;
	re = ldap_set_option (conn, LDAP_OPT_PROTOCOL_VERSION, &val);
	if (re != LDAP_OPT_SUCCESS)
		goto error;

	if (props->tls) {
#ifdef LDAP_HAVE_START_TLS_S
		re = ldap_start_tls_s (conn, NULL,  NULL);
		if (re != LDAP_OPT_SUCCESS) {
			TRACE (ENTRIES, "Couldn't StartTLS\n");
			goto error;
		}
#else
		PRINT_ERROR_S ("Can't StartTLS, it isn't supported by LDAP client libraries\n");
#endif
	}

	re = ldap_simple_bind_s (conn, dn, password);
	if (re != LDAP_SUCCESS)
		goto error;

	return ret_ok;

error:
	ldap_unbind_s (conn);
	return ret_error;
}


static ret_t
init_filter (cherokee_validator_ldap_t *ldap, cherokee_validator_ldap_props_t *props, cherokee_connection_t *conn)
{
	cherokee_buffer_ensure_size (&ldap->filter, props->filter.len + conn->validator->user.len);
	cherokee_buffer_add_buffer (&ldap->filter, &props->filter);
	cherokee_buffer_replace_string (&ldap->filter, "${user}", 7, conn->validator->user.buf, conn->validator->user.len);

	TRACE (ENTRIES, "filter %s\n", ldap->filter.buf);

	return ret_ok;
}


ret_t 
cherokee_validator_ldap_check (cherokee_validator_ldap_t *ldap, cherokee_connection_t *conn)
{
	int                              re;
	ret_t                            ret;
	size_t                           size;
	char                            *dn;
	LDAPMessage                     *message;
	LDAPMessage                     *first;
	char                            *attrs[] = { LDAP_NO_ATTRS, NULL };
	cherokee_validator_ldap_props_t *props   = VAL_LDAP_PROP(ldap);

	/* Sanity checks
	 */
	if ((conn->validator == NULL) ||
	    cherokee_buffer_is_empty (&conn->validator->user))
		return ret_error;

	size = cherokee_buffer_cnt_cspn (&conn->validator->user, 0, "*()");
	if (size != conn->validator->user.len)
		return ret_error;

	/* Build filter
	 */
	ret = init_filter (ldap, props, conn);
	if (ret != ret_ok)
		return ret;

	/* Search
	 */
	re = ldap_search_s (ldap->conn, props->basedn.buf, LDAP_SCOPE_SUBTREE, ldap->filter.buf, attrs, 0, &message);
	if (re != LDAP_SUCCESS) {
		PRINT_ERROR ("Couldn't search in LDAP server: %s\n", props->filter.buf);
		return ret_error;
	}

	TRACE (ENTRIES, "subtree search (%s): done\n", ldap->filter.buf);

	/* Check that there a single entry
	 */
	re = ldap_count_entries (ldap->conn, message);
	if (re != 1) {
		ldap_msgfree (message);
		return ret_not_found;		
	}

	/* Pick up the first one
	 */
	first = ldap_first_entry (ldap->conn, message);
	if (first == NULL) {
		ldap_msgfree (message);
		return ret_not_found;
	}

	/* Get DN
	 */
	dn = ldap_get_dn (ldap->conn, first);
	if (dn == NULL) {
		ldap_msgfree (message);
		return ret_error;
	}

	ldap_msgfree (message);

	/* Check that it's right
	 */
	ret = validate_dn (props, dn, conn->validator->passwd.buf);
	if (ret != ret_ok)
		return ret;

	/* Disconnect from the LDAP server
	 */
	re = ldap_unbind_s (ldap->conn);
	if (re != LDAP_SUCCESS)
		return ret_error;

	/* Validated!
	 */
	TRACE (ENTRIES, "Access to use %s has been granted\n", conn->validator->user.buf);

	return ret_ok;
}

ret_t 
cherokee_validator_ldap_add_headers (cherokee_validator_ldap_t *ldap, cherokee_connection_t *conn, cherokee_buffer_t *buf)
{
	UNUSED(ldap);
	UNUSED(conn);
	UNUSED(buf);

	return ret_ok;
}

