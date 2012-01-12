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

#include "common-internal.h"
#include "connection.h"


ret_t
cherokee_connection_init (cherokee_connection_t *conn)
{
	conn->keepalive = 0;

	cherokee_socket_init (&conn->socket);
	cherokee_connection_poll_init (&conn->polling_aim);

	INIT_LIST_HEAD (&conn->requests);
	return ret_ok;
}


static void
reuse_requests (cherokee_connection_t *conn)
{
	cherokee_thread_t *thd      = CONN_THREAD(conn);
	cherokee_list_t   *i, *tmp;

	list_for_each_safe (i, tmp, &conn->requests) {
		if (thd != NULL) {
			cherokee_thread_recycle_request (thd, REQ(i));
		} else {
			cherokee_request_free (REQ(i));
		}
	}
}


ret_t
cherokee_connection_mrproper (cherokee_connection_t *conn)
{
	/* Polling aim */
	if (conn->polling_aim.fd != -1) {
                cherokee_fd_close (conn->polling_aim.fd);
	}

	cherokee_connection_poll_mrproper (&conn->polling_aim);

	/* Connection socket */
	cherokee_socket_mrproper (&conn->socket);

	/* Free requests */
	reuse_requests (conn);

	return ret_ok;
}


ret_t
cherokee_connection_clean (cherokee_connection_t *conn)
{
	cherokee_list_t *i, *tmp;

	/* Polling aim */
	if (conn->polling_aim.fd != -1) {
                cherokee_fd_close (conn->polling_aim.fd);
	}

	cherokee_connection_poll_clean (&conn->polling_aim);

	/* Free requests */
	reuse_requests (conn);

	return ret_ok;
}
