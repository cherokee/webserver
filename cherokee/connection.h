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

#ifndef CHEROKEE_REQUEST_H
#define CHEROKEE_REQUEST_H

#include "common.h"
#include "buffer.h"
#include "socket.h"
#include "list.h"
#include "connection-poll.h"

CHEROKEE_BEGIN_DECLS

typedef struct {
	cherokee_socket_t          socket;
	uint32_t                   keepalive;
	cherokee_connection_pool_t polling_aim;
	cherokee_list_t            requests;
} cherokee_connection_t;

#define CONN(c) ((cherokee_connection_t *)(c))


/* Public methods
 */
ret_t cherokee_connection_init     (cherokee_connection_t *conn);
ret_t cherokee_connection_mrproper (cherokee_connection_t *conn);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_REQUEST_H */
