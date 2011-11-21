/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_VALIDATOR_DUMMY_H
#define CHEROKEE_VALIDATOR_DUMMY_H

#include "validator.h"
#include "connection.h"

typedef struct {
	cherokee_module_props_t   base;
} cherokee_validator_dummy_props_t;

typedef struct {
	cherokee_validator_t      validator;
} cherokee_validator_dummy_t;

/* Validator
 */

ret_t cherokee_validator_dummy_new (cherokee_validator_dummy_t **dummy,
                                    cherokee_module_props_t     *props);

ret_t cherokee_validator_dummy_configure (cherokee_config_node_t    *conf,
					  cherokee_server_t         *srv,
					  cherokee_module_props_t **_props);

ret_t cherokee_validator_dummy_check (cherokee_validator_dummy_t *dummy,
                                      cherokee_connection_t      *conn);

#endif /* CHEROKEE_VALIDATOR_DUMMY_H */
