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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_CONNECTION_H
#define CHEROKEE_CONNECTION_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>


CHEROKEE_BEGIN_DECLS

typedef struct cherokee_connection cherokee_connection_t;
#define CONN(c)        ((cherokee_connection_t *)(c))


/* Public methods
 */
ret_t  cherokee_connection_set_cork      (cherokee_connection_t *conn, cherokee_boolean_t enable);
ret_t  cherokee_connection_parse_args    (cherokee_connection_t *conn);

/* Handy functions
 */
const char *cherokee_connection_get_phase_str (cherokee_connection_t *conn);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_CONNECTION_H */
