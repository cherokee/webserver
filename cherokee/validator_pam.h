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

#ifndef CHEROKEE_VALIDATOR_PAM_H
#define CHEROKEE_VALIDATOR_PAM_H

#include "validator.h"
#include "connection.h"
#include "plugin_loader.h"

typedef struct {
	cherokee_validator_t validator;
} cherokee_validator_pam_t;

#define PAM(x) ((cherokee_validator_pam_t *)(x))

void PLUGIN_INIT_NAME(pam) (cherokee_plugin_loader_t *loader);

ret_t cherokee_validator_pam_new  (cherokee_validator_pam_t **pam, cherokee_module_props_t *props);
ret_t cherokee_validator_pam_free (cherokee_validator_pam_t  *pam);

ret_t cherokee_validator_pam_check       (cherokee_validator_pam_t  *pam, cherokee_connection_t *conn);
ret_t cherokee_validator_pam_add_headers (cherokee_validator_pam_t  *pam, cherokee_connection_t *conn, cherokee_buffer_t *buf);

#endif /* CHEROKEE_VALIDATOR_PAM_H */
