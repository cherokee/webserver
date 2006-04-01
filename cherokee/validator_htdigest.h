/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
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

#ifndef CHEROKEE_VALIDATOR_HTDIGEST_H
#define CHEROKEE_VALIDATOR_HTDIGEST_H

#include "validator.h"
#include "connection.h"

typedef struct {
	   cherokee_validator_t  validator;
	   char                 *file_ref;
} cherokee_validator_htdigest_t;

#define HTDIGEST(x) ((cherokee_validator_htdigest_t *)(x))


ret_t cherokee_validator_htdigest_new  (cherokee_validator_htdigest_t **htdigest, cherokee_table_t *properties);
ret_t cherokee_validator_htdigest_free (cherokee_validator_htdigest_t  *htdigest);

ret_t cherokee_validator_htdigest_check       (cherokee_validator_htdigest_t *htdigest, cherokee_connection_t *conn);
ret_t cherokee_validator_htdigest_add_headers (cherokee_validator_htdigest_t *htdigest, cherokee_connection_t *conn, cherokee_buffer_t *buf);

#endif /* CHEROKEE_VALIDATOR_HTDIGEST_H */
