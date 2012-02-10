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
#include "protocol_http.h"

ret_t
cherokee_protocol_http_dispatch (cherokee_protocol_t *proto)
{
	size_t                 bytes  = 0;
	cherokee_connection_t *conn   = CONN (PROTOCOL_BASE(proto)->conn);
	cherokee_fdpoll_t     *fdpoll = CONN_THREAD(conn)->fdpoll;
	cherokee_connection_t *sock   = SOCKET (&conn->socket);
	cherokee_poll_mode_t   mode   = poll_mode_nothing;
	cherokee_poll_mode_t   mode_s = poll_mode_nothing;

	/* Figure out the mode
	 */
	if (! cherokee_buffer_is_empty (conn->buffer_out)) {
		mode |= poll_mode_write;
	}

	mode_s = cherokee_fdpoll_check (fdpoll, conn->socket.socket);
	if (mode_s & poll_mode_read) {
		mode |= poll_mode_read;
	}

	/* Send
	 */
	if (mode & poll_mode_write) {
		/* Punch buffer in the socket */
		ret = cherokee_socket_bufwrite (sock, conn->buffer_out, &bytes);

		switch (ret) {
		case ret_ok:
			break;

		case ret_eagain:
			conn->polling_aim.fd   = conn->socket.socket;
			conn->polling_aim.mode = poll_mode_write;
			goto receive;

		default:
			return ret;
		}

		/* Skip the chunk already sent */
		cherokee_buffer_move_to_begin (conn->buffer_out, bytes);
	}

	/* Receive */
receive:
	if (mode & poll_mode_read) {
		bytes = 0;

		ret = cherokee_socket_bufread (sock, conn->buffer_in, to_read, &bytes);
		switch (ret) {
		case ret_ok:
			cherokee_connection_rx_add (conn, cnt_read);
			*len = cnt_read;
			return ret_ok;

	case ret_eof:
	case ret_error:
		return ret;

	case ret_eagain:
		if (cnt_read > 0) {
			cherokee_connection_rx_add (conn, cnt_read);
			*len = cnt_read;
			return ret_ok;
		}

		conn->polling_aim.fd   = conn->socket.socket;
		conn->polling_aim.mode = poll_mode_read;

		return ret_eagain;

	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}


	}


	if (cherokee_connection_poll_is_set (&conn->polling_aim))
		return ret_egain;

	return ret_ok;
}
