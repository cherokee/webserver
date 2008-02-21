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

#ifndef CHEROKEE_VALIDATOR_LDAP_H
#define CHEROKEE_VALIDATOR_LDAP_H

#include "common.h"

#include "ldap.h"

#include "validator.h"
#include "connection.h"


typedef struct {
	cherokee_module_props_t    base;

	cherokee_buffer_t          server;
	cint_t                     port;

	cherokee_buffer_t          binddn;
	cherokee_buffer_t          bindpw;
	cherokee_buffer_t          basedn;
	cherokee_buffer_t          filter;

	cherokee_boolean_t         tls;
	cherokee_buffer_t          ca_file;
} cherokee_validator_ldap_props_t;

typedef struct {
	cherokee_validator_t       validator;
	LDAP                      *conn;
	cherokee_buffer_t          filter;
} cherokee_validator_ldap_t;

#define LDAP(x)          ((cherokee_validator_ldap_t *)(x))
#define PROP_LDAP(p)     ((cherokee_validator_ldap_props_t *)(p))
#define VAL_LDAP_PROP(x) (PROP_LDAP(MODULE(x)->props))


ret_t cherokee_validator_ldap_new         (cherokee_validator_ldap_t **ldap, cherokee_module_props_t *props);
ret_t cherokee_validator_ldap_free        (cherokee_validator_ldap_t  *ldap);

ret_t cherokee_validator_ldap_check       (cherokee_validator_ldap_t  *ldap, cherokee_connection_t *conn);
ret_t cherokee_validator_ldap_add_headers (cherokee_validator_ldap_t  *ldap, cherokee_connection_t *conn, cherokee_buffer_t *buf);

#endif /* CHEROKEE_VALIDATOR_LDAP_H */
