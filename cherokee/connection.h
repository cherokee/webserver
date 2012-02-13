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

#ifndef CHEROKEE_CONNECTION_H
#define CHEROKEE_CONNECTION_H

#include "common.h"
#include "buffer.h"
#include "socket.h"
#include "list.h"
#include "protocol.h"
#include "connection-poll.h"

CHEROKEE_BEGIN_DECLS

typedef struct {
	/* Inherits from list */
	cherokee_list_t             list_entry;

	/* Pointers */
	void                       *thread;

	/* Properties */
	cherokee_socket_t           socket;
	cherokee_protocol_t         protocol;
	cherokee_connection_pool_t  polling_aim;
	cherokee_list_t             requests;

	/* Timeout & Keep-alive */
	time_t                      timeout;
	time_t                      timeout_lapse;
	cherokee_buffer_t          *timeout_header;
	uint32_t                    keepalive;

	/* Buffers */
	cherokee_buffer_t           buffer_in;
	cherokee_buffer_t           buffer_out;

	/* Traffic */
	off_t                       rx;                  /* Bytes received */
	size_t                      rx_partial;          /* RX partial counter */
	off_t                       tx;                  /* Bytes sent */
	size_t                      tx_partial;          /* TX partial counter */
	time_t                      traffic_next;        /* Time to update traffic */

} cherokee_connection_t;

#define CONN(c)        ((cherokee_connection_t *)(c))
#define CONN_THREAD(c) (CONN(c)->thread)

/* Public methods
 */
ret_t cherokee_connection_init     (cherokee_connection_t *conn);
ret_t cherokee_connection_mrproper (cherokee_connection_t *conn);

/* Transfers
 */
void cherokee_connection_rx_add    (cherokee_connection_t *conn, ssize_t rx);
void cherokee_connection_tx_add    (cherokee_connection_t *conn, ssize_t tx);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_CONNECTION_H */
