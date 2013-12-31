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

#include "common-internal.h"
#include "limiter.h"
#include "connection-protected.h"
#include "bogotime.h"
#include "thread.h"

ret_t
cherokee_limiter_init (cherokee_limiter_t *limiter)
{
	INIT_LIST_HEAD (&limiter->conns);
	limiter->conns_num = 0;

	return ret_ok;
}


ret_t
cherokee_limiter_mrproper (cherokee_limiter_t *limiter)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, &limiter->conns) {
		cherokee_connection_free (CONN(i));
	}

	return ret_ok;
}


ret_t
cherokee_limiter_add_conn (cherokee_limiter_t *limiter,
                           void               *conn)
{
	cherokee_list_t *i;
	cherokee_list_t *prev;

	/* Enlist the connection. Shorted insert by unblocking order
	 */
	prev = &limiter->conns;
	list_for_each (i, &limiter->conns) {
		if (CONN(i)->limit_blocked_until > CONN(conn)->limit_blocked_until)
			break;

		prev = i;
	}

	limiter->conns_num++;
	cherokee_list_add (LIST(conn), prev);

	return ret_ok;
}


cherokee_msec_t
cherokee_limiter_get_time_limit (cherokee_limiter_t *limiter,
                                 cherokee_msec_t     fdwatch_msecs)
{
	long long              time_delta;
	cherokee_connection_t *conn;

	/* Shortcut
	 */
	if ((fdwatch_msecs == 0) ||
	    (limiter->conns_num == 0))
	{
		return fdwatch_msecs;
	}

	/* Pick the first connection
	 */
	conn = CONN(limiter->conns.next);

	/* Return the delta time of the first connection (the
	 * connection that will be enabled sooner)
	 */
	time_delta = conn->limit_blocked_until - cherokee_bogonow_msec;
	if (unlikely (time_delta <= 0)) {
		return 0;
	}

	/* Check as 'signed long long' */
	return MIN(time_delta, (long long)fdwatch_msecs);
}


ret_t
cherokee_limiter_reactive (cherokee_limiter_t *limiter,
                           void               *thread)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, &limiter->conns) {
		/* Check whether the limit has expired
		 */
		if (CONN(i)->limit_blocked_until > cherokee_bogonow_msec)
			break;

		/* Re-active the connection
		 */
		limiter->conns_num--;
		cherokee_list_del(i);

		CONN(i)->limit_blocked_until = 0;
		cherokee_thread_inject_active_connection (thread, CONN(i));
	}

	return ret_ok;
}
