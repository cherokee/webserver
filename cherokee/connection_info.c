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

#include "connection_info.h"
#include "connection-protected.h"
#include "thread.h"
#include "server-protected.h"
#include "handler_file.h"


ret_t
cherokee_connection_info_new (cherokee_connection_info_t **info)
{
	CHEROKEE_NEW_STRUCT(n, connection_info);

	INIT_LIST_HEAD(&n->list_node);

	cherokee_buffer_init (&n->id);
	cherokee_buffer_init (&n->phase);
	cherokee_buffer_init (&n->request);
	cherokee_buffer_init (&n->rx);
	cherokee_buffer_init (&n->tx);
	cherokee_buffer_init (&n->total_size);
	cherokee_buffer_init (&n->ip);
	cherokee_buffer_init (&n->percent);
	cherokee_buffer_init (&n->handler);
	cherokee_buffer_init (&n->icon);

	*info = n;
	return ret_ok;
}


ret_t
cherokee_connection_info_free (cherokee_connection_info_t *info)
{
	cherokee_buffer_mrproper (&info->id);
	cherokee_buffer_mrproper (&info->phase);
	cherokee_buffer_mrproper (&info->request);
	cherokee_buffer_mrproper (&info->rx);
	cherokee_buffer_mrproper (&info->tx);
	cherokee_buffer_mrproper (&info->total_size);
	cherokee_buffer_mrproper (&info->ip);
	cherokee_buffer_mrproper (&info->percent);
	cherokee_buffer_mrproper (&info->handler);
	cherokee_buffer_mrproper (&info->icon);

	free (info);
	return ret_ok;
}


static ret_t
info_fill_up (cherokee_connection_info_t *info,
	      cherokee_connection_t      *conn)
{
	ret_t             ret;
	const char       *phase        = NULL;
	const char       *handler_name = NULL;
	cherokee_icons_t *icons        = CONN_SRV(conn)->icons;

	/* ID
	 */
	cherokee_buffer_add_va (&info->id, "%lu", conn->id);

	/* Phase
	 */
	phase = cherokee_connection_get_phase_str (conn);
	cherokee_buffer_add (&info->phase, phase, strlen(phase));

	/* From
	 */
	if (conn->socket.socket >= 0) {
		cherokee_buffer_ensure_size (&info->ip, CHE_INET_ADDRSTRLEN + 1);
		memset (info->ip.buf, 0, info->ip.size);
		cherokee_socket_ntop (&conn->socket, info->ip.buf, info->ip.size - 1);
		info->ip.len = strlen(info->ip.buf);
	}

	/* Request
	 */
	if (! cherokee_buffer_is_empty (&conn->request_original)) {
		cherokee_buffer_add_buffer (&info->request, &conn->request_original);

	} else if (! cherokee_buffer_is_empty (&conn->request)) {
		cherokee_buffer_add_buffer (&info->request, &conn->request);
	}

	/* Transference
	 */
	cherokee_buffer_add_va (&info->rx, FMT_OFFSET, (CST_OFFSET)conn->rx);
	cherokee_buffer_add_va (&info->tx, FMT_OFFSET, (CST_OFFSET)conn->tx);

	/* Handler
	 */
	if (conn->handler != NULL) {
		cherokee_module_get_name (MODULE(conn->handler), &handler_name);

		if (handler_name)
			cherokee_buffer_add (&info->handler, handler_name, strlen(handler_name));
	}

	/* Total size
	 */
	if (handler_name && !strcmp (handler_name, "file")) {
		char                    *point;
		double                   percent;
		cherokee_handler_file_t *file = HDL_FILE(conn->handler);

		/* File size
		 */
		cherokee_buffer_add_va (&info->total_size, FMT_OFFSET, (CST_OFFSET)file->info->st_size);

		/* Percent
		 */
		percent = (CST_OFFSET)conn->tx * (double)100 / (CST_OFFSET)file->info->st_size;
		cherokee_buffer_add_va (&info->percent, "%f", percent);

		point = strchr (info->percent.buf, '.');
		if (point != NULL)
			cherokee_buffer_drop_ending (&info->percent, (info->percent.buf + info->percent.len) - (point + 2));
	}

	/* Local icon
	 */
	if ((icons != NULL) &&
	    (!cherokee_buffer_is_empty (&info->request)))
	{
		char              *tmp;
		cherokee_buffer_t *icon;
		cherokee_buffer_t  name = CHEROKEE_BUF_INIT;

		cherokee_buffer_add_buffer (&name, &info->request);

		tmp = strchr (name.buf, '?');
		if (tmp != NULL)
			cherokee_buffer_drop_ending (&name, (name.buf + name.len) - tmp);

		tmp = strrchr (name.buf, '/');
		if (tmp != NULL)
			cherokee_buffer_move_to_begin (&name, tmp - name.buf);

		ret = cherokee_icons_get_icon (icons, &name, &icon);
		if (ret == ret_ok)
			cherokee_buffer_add_buffer (&info->icon, icon);

		cherokee_buffer_mrproper (&name);
	}

	return ret_ok;
}


ret_t
cherokee_connection_info_list_thread (cherokee_list_t    *list,
                                      void               *_thread,
                                      cherokee_handler_t *self_handler)
{
	ret_t               ret;
	cherokee_list_t    *i;
	cherokee_boolean_t  locked = false;
	cherokee_thread_t  *thread = THREAD(_thread);

	/* Does it has active connections?
	 */
	if (thread->active_list_num <= 0)
		return ret_not_found;

	/* If it tries to adquire the thread ownership of the thread
	 * which is already calling this function it will generate
	 * a deadlock situation.  Check it before lock.
	 */
	if ((self_handler != NULL) &&
	    (HANDLER_THREAD(self_handler) != thread)) {
		/* Adquire the ownership of the thread
		 */
		CHEROKEE_MUTEX_LOCK (&thread->ownership);
		locked = true;
	}

	/* Process each connection
	 */
	list_for_each (i, &thread->active_list) {
		cherokee_connection_info_t *n;

		ret = cherokee_connection_info_new (&n);
		if (unlikely (ret != ret_ok)) goto out;

		info_fill_up (n, CONN(i));
		cherokee_list_add (LIST(n), list);
	}

	list_for_each (i, &thread->polling_list) {
		cherokee_connection_info_t *n;

		ret = cherokee_connection_info_new (&n);
		if (unlikely (ret != ret_ok)) goto out;

		info_fill_up (n, CONN(i));
		cherokee_list_add (LIST(n), list);
	}

	list_for_each (i, &thread->limiter.conns) {
		cherokee_connection_info_t *n;

		ret = cherokee_connection_info_new (&n);
		if (unlikely (ret != ret_ok)) goto out;

		info_fill_up (n, CONN(i));
		cherokee_list_add (LIST(n), list);
	}

	ret = ret_ok;
	if (cherokee_list_empty (list))
		ret = ret_not_found;

out:
	/* Release it
	 */
	if (locked)
		CHEROKEE_MUTEX_UNLOCK (&thread->ownership);

	return ret;
}


ret_t
cherokee_connection_info_list_server (cherokee_list_t    *list,
                                      cherokee_server_t  *server,
                                      cherokee_handler_t *self)
{
	cherokee_list_t *i;

	cherokee_connection_info_list_thread (list, server->main_thread, self);

	list_for_each (i, &server->thread_list) {
		cherokee_connection_info_list_thread (list, i, self);
	}

	if (cherokee_list_empty (list))
		return ret_not_found;

	return ret_ok;
}
