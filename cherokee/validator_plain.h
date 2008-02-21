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

#ifndef CHEROKEE_VALIDATOR_PLAIN_H
#define CHEROKEE_VALIDATOR_PLAIN_H

#include "validator.h"
#include "connection.h"

typedef struct {
	cherokee_module_props_t  base;
	cherokee_buffer_t        password_file;
} cherokee_validator_plain_props_t;

typedef struct {
	cherokee_validator_t  validator;
} cherokee_validator_plain_t;

#define PLAIN(x)          ((cherokee_validator_plain_t *)(x))
#define PROP_PLAIN(p)     ((cherokee_validator_plain_props_t *)(p))
#define VAL_PLAIN_PROP(x) (PROP_PLAIN (MODULE(x)->props))


ret_t cherokee_validator_plain_new         (cherokee_validator_plain_t **plain, cherokee_module_props_t *props);
ret_t cherokee_validator_plain_free        (cherokee_validator_plain_t  *plain);

ret_t cherokee_validator_plain_check       (cherokee_validator_plain_t  *plain, cherokee_connection_t *conn);
ret_t cherokee_validator_plain_add_headers (cherokee_validator_plain_t  *plain, cherokee_connection_t *conn, cherokee_buffer_t *buf);

#endif /* CHEROKEE_VALIDATOR_PLAIN_H */
