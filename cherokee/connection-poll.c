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
#include "connection-poll.h"

ret_t
cherokee_connection_poll_init (cherokee_connection_pool_t *conn_poll)
{
	   conn_poll->fd   = -1;
	   conn_poll->mode = poll_mode_nothing;

	   return ret_ok;
}

ret_t
cherokee_connection_poll_mrproper (cherokee_connection_pool_t *conn_poll)
{
	   UNUSED (conn_poll);
	   return ret_ok;
}

ret_t
cherokee_connection_poll_clean (cherokee_connection_pool_t *conn_poll)
{
	   conn_poll->fd   = -1;
	   conn_poll->mode = poll_mode_nothing;

	   return ret_ok;
}
