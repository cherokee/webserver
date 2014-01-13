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
#include "admin_server.h"

#include <signal.h>

#include "bind.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "connection_info.h"
#include "source_interpreter.h"
#include "util.h"


ret_t
cherokee_admin_server_reply_get_ports (cherokee_handler_t *hdl,
                                       cherokee_dwriter_t  *dwriter)
{
	cherokee_list_t   *i;
	cherokee_bind_t   *bind_entry;
	cherokee_server_t *srv         = HANDLER_SRV(hdl);

	cherokee_dwriter_list_open (dwriter);

	list_for_each (i, &srv->listeners) {
		bind_entry = BIND(i);

		cherokee_dwriter_dict_open (dwriter);

		cherokee_dwriter_cstring (dwriter, "id");
		cherokee_dwriter_unsigned (dwriter, bind_entry->id);

		cherokee_dwriter_cstring (dwriter, "bind");
		cherokee_dwriter_bstring (dwriter, &bind_entry->ip);

		cherokee_dwriter_cstring (dwriter, "port");
		cherokee_dwriter_unsigned (dwriter, bind_entry->port);

		cherokee_dwriter_cstring (dwriter, "tls");
		cherokee_dwriter_bool    (dwriter, BIND_IS_TLS(bind_entry));

		cherokee_dwriter_dict_close (dwriter);
	}
	cherokee_dwriter_list_close (dwriter);

	return ret_ok;
}

ret_t
cherokee_admin_server_reply_get_traffic (cherokee_handler_t *hdl,
                                         cherokee_dwriter_t *dwriter)
{
	cherokee_server_t *srv = HANDLER_SRV(hdl);
	cherokee_buffer_t *tmp = THREAD_TMP_BUF2 (HANDLER_THREAD(hdl));

	cherokee_dwriter_dict_open (dwriter);

	cherokee_dwriter_cstring (dwriter, "tx");
	if (srv->collector) {
		cherokee_dwriter_unsigned (dwriter, COLLECTOR_TX(srv->collector));
	} else {
		cherokee_dwriter_number (dwriter, "-1", 2);
	}

	cherokee_dwriter_cstring (dwriter, "rx");
	if (srv->collector) {
		cherokee_dwriter_unsigned (dwriter, COLLECTOR_RX(srv->collector));
	} else {
		cherokee_dwriter_number (dwriter, "-1", 2);
	}

	cherokee_dwriter_cstring (dwriter, "tx_formatted");
	if (srv->collector != NULL) {
		cherokee_buffer_clean     (tmp);
		cherokee_buffer_add_fsize (tmp, COLLECTOR_TX(srv->collector));
		cherokee_dwriter_bstring (dwriter, tmp);
	} else {
		cherokee_dwriter_null (dwriter);
	}

	cherokee_dwriter_cstring (dwriter, "rx_formatted");
	if (srv->collector != NULL) {
		cherokee_buffer_clean     (tmp);
		cherokee_buffer_add_fsize (tmp, COLLECTOR_RX(srv->collector));
		cherokee_dwriter_bstring (dwriter, tmp);
	} else {
		cherokee_dwriter_null (dwriter);
	}

	cherokee_dwriter_dict_close (dwriter);
	return ret_ok;
}


static void
render_connection_info (cherokee_connection_info_t *conn_info,
                        cherokee_dwriter_t         *dwriter)
{
	cherokee_dwriter_dict_open (dwriter);

	cherokee_dwriter_cstring (dwriter, "id");
	cherokee_dwriter_bstring (dwriter, &conn_info->id);
	cherokee_dwriter_cstring (dwriter, "ip");
	cherokee_dwriter_bstring (dwriter, &conn_info->ip);
	cherokee_dwriter_cstring (dwriter, "phase");
	cherokee_dwriter_bstring (dwriter, &conn_info->phase);

	if (! cherokee_buffer_is_empty(&conn_info->rx)) {
		cherokee_dwriter_cstring (dwriter, "rx");
		cherokee_dwriter_bstring (dwriter, &conn_info->rx);
	}
	if (! cherokee_buffer_is_empty(&conn_info->tx)) {
		cherokee_dwriter_cstring (dwriter, "tx");
		cherokee_dwriter_bstring (dwriter, &conn_info->tx);
	}
	if (! cherokee_buffer_is_empty(&conn_info->request)) {
		cherokee_dwriter_cstring (dwriter, "request");
		cherokee_dwriter_bstring (dwriter, &conn_info->request);
	}
	if (! cherokee_buffer_is_empty(&conn_info->handler)) {
		cherokee_dwriter_cstring (dwriter, "handler");
		cherokee_dwriter_bstring (dwriter, &conn_info->handler);
	}
	if (! cherokee_buffer_is_empty(&conn_info->total_size)) {
		cherokee_dwriter_cstring (dwriter, "total_size");
		cherokee_dwriter_bstring (dwriter, &conn_info->total_size);
	}
	if (! cherokee_buffer_is_empty(&conn_info->percent)) {
		cherokee_dwriter_cstring (dwriter, "percent");
		cherokee_dwriter_bstring (dwriter, &conn_info->percent);
	}
	if (! cherokee_buffer_is_empty(&conn_info->icon)) {
		cherokee_dwriter_cstring (dwriter, "icon");
		cherokee_dwriter_bstring (dwriter, &conn_info->icon);
	}

	cherokee_dwriter_dict_close (dwriter);
}

ret_t
cherokee_admin_server_reply_get_conns (cherokee_handler_t *hdl,
                                       cherokee_dwriter_t *dwriter)
{
	ret_t              ret;
	cherokee_list_t    connections;
	cherokee_list_t   *i, *tmp;
	cherokee_server_t *srv          = HANDLER_SRV(hdl);

	/* Build the connection-info list
	 */
	INIT_LIST_HEAD (&connections);

	ret = cherokee_connection_info_list_server (&connections, srv, HANDLER(hdl));
	if (unlikely (ret == ret_error)) {
		return ret_error;
	}

	/* Render it
	 */
	cherokee_dwriter_list_open (dwriter);
	list_for_each (i, &connections) {
		cherokee_connection_info_t *conn_info = CONN_INFO(i);

		/* It won't include details about the admin requests
		 */
		if (! cherokee_buffer_cmp_str (&conn_info->handler, "admin")) {
			continue;
		}

		render_connection_info (conn_info, dwriter);
	}
	cherokee_dwriter_list_close (dwriter);

	/* Free the connection info objects
	 */
	list_for_each_safe (i, tmp, &connections) {
		cherokee_connection_info_free (CONN_INFO(i));
	}

	return ret_ok;
}


ret_t
cherokee_admin_server_reply_close_conn (cherokee_handler_t *hdl,
                                        cherokee_dwriter_t *dwriter,
                                        cherokee_buffer_t  *question)
{
	ret_t              ret;
	char              *begin;
	cherokee_server_t *server = HANDLER_SRV(hdl);
	cherokee_buffer_t  match  = CHEROKEE_BUF_INIT;

	cherokee_buffer_fake_str (&match, "close server.connection ");

	if (strncasecmp (question->buf, match.buf, match.len)) {
		return ret_error;
	}

	begin = question->buf + match.len;
	ret = cherokee_server_close_connection (server, CONN_THREAD(HANDLER_CONN(hdl)), begin);

	cherokee_dwriter_dict_open (dwriter);
	cherokee_dwriter_cstring (dwriter, "close");
	if (ret == ret_ok) {
		cherokee_dwriter_cstring (dwriter, "ok");
	} else {
		cherokee_dwriter_cstring (dwriter, "failed");
	}
	cherokee_dwriter_dict_close (dwriter);

	return ret_ok;
}


ret_t
cherokee_admin_server_reply_get_trace (cherokee_handler_t *hdl,
                                       cherokee_dwriter_t *dwriter)
{
	ret_t              ret;
	cherokee_buffer_t *traces_ref = NULL;

	UNUSED (hdl);

	ret = cherokee_trace_get_trace (&traces_ref);
	if (ret != ret_ok) {
		return ret_error;
	}

	cherokee_dwriter_dict_open (dwriter);
	cherokee_dwriter_cstring   (dwriter, "trace");

	if (cherokee_buffer_is_empty (traces_ref)) {
		cherokee_dwriter_null (dwriter);
	} else {
		cherokee_dwriter_bstring (dwriter, traces_ref);
	}

	cherokee_dwriter_dict_close (dwriter);
	return ret_ok;
}

ret_t
cherokee_admin_server_reply_set_trace (cherokee_handler_t *hdl,
                                       cherokee_dwriter_t *dwriter,
                                       cherokee_buffer_t  *question)
{
	ret_t ret;
	cherokee_buffer_t match = CHEROKEE_BUF_INIT;

	UNUSED (hdl);

	cherokee_buffer_fake_str (&match, "set server.trace ");

	/* Process question
	 */
	if (strncasecmp (question->buf, match.buf, match.len)) {
		return ret_error;
	}
	cherokee_buffer_move_to_begin (question, match.len);

	/* Set the traces
	 */
	ret = cherokee_trace_set_modules (question);
	if (ret != ret_ok) {
		return ret_error;
	}

	/* Reply
	 */
	cherokee_dwriter_dict_open (dwriter);
	cherokee_dwriter_cstring (dwriter, "set");
	cherokee_dwriter_bool (dwriter, ret == ret_ok);
	cherokee_dwriter_dict_close (dwriter);
	return ret_ok;
}


ret_t
cherokee_admin_server_reply_set_backup_mode (cherokee_handler_t *hdl,
                                             cherokee_dwriter_t *dwriter,
                                             cherokee_buffer_t  *question)
{
	ret_t              ret;
	cherokee_server_t *srv = HANDLER_SRV(hdl);
	cherokee_boolean_t active;
	cherokee_boolean_t mode;

	/* Read if the resquest if for turning it on or off
	 */
	if (cherokee_buffer_cmp_str (question, "set server.backup_mode on") == 0) {
		mode = true;
	} else if (cherokee_buffer_cmp_str (question, "set server.backup_mode off") == 0) {
		mode = false;
	} else {
		return ret_error;
	}

	/* Do it
	 */
	ret = cherokee_server_set_backup_mode (srv, mode);
	if (ret != ret_ok) return ret;

	/* Build the reply
	 */
	cherokee_server_get_backup_mode (srv, &active);

	cherokee_dwriter_dict_open (dwriter);
	cherokee_dwriter_cstring (dwriter, "backup_mode");
	cherokee_dwriter_bool (dwriter, mode);
	cherokee_dwriter_dict_close (dwriter);

	return ret_ok;
}

ret_t
cherokee_admin_server_reply_get_thread_num (cherokee_handler_t *hdl,
                                            cherokee_dwriter_t *dwriter)
{
	cherokee_server_t *srv = HANDLER_SRV(hdl);

	cherokee_dwriter_dict_open (dwriter);
	cherokee_dwriter_cstring (dwriter, "thread_num");
	cherokee_dwriter_unsigned (dwriter, srv->thread_num);
	cherokee_dwriter_dict_close (dwriter);

	return ret_ok;
}


static ret_t
sources_while (cherokee_buffer_t *key, void *value, void *param)
{
	cherokee_dwriter_t *dwriter = DWRITER(param);
	cherokee_source_t  *source  = SOURCE(value);

	cherokee_dwriter_dict_open (dwriter);

	cherokee_dwriter_cstring (dwriter, "id");
	cherokee_dwriter_bstring (dwriter, key);

	cherokee_dwriter_cstring (dwriter, "type");
	switch (source->type) {
	case source_host:
		cherokee_dwriter_cstring (dwriter, "host");
		break;
	case source_interpreter:
		cherokee_dwriter_cstring (dwriter, "interpreter");
		break;
	default:
		cherokee_dwriter_dict_close (dwriter);
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	cherokee_dwriter_cstring (dwriter, "bind");
	cherokee_dwriter_bstring (dwriter, &source->original);

	if (source->type == source_interpreter) {
		cherokee_source_interpreter_t *source_int = SOURCE_INT(source);

		cherokee_dwriter_cstring (dwriter, "PID");
		if (source_int->pid == -1) {
			cherokee_dwriter_number   (dwriter, "-1", 2);
		} else {
			cherokee_dwriter_unsigned (dwriter, source_int->pid);
		}

		cherokee_dwriter_cstring (dwriter, "debug");
		cherokee_dwriter_bool    (dwriter, source_int->debug);

		cherokee_dwriter_cstring (dwriter, "timeout");
		cherokee_dwriter_unsigned (dwriter, source_int->timeout);

		cherokee_dwriter_cstring (dwriter, "interpreter");
		cherokee_dwriter_bstring (dwriter, &source_int->interpreter);
	}

	cherokee_dwriter_dict_close (dwriter);
	return ret_ok;
}

ret_t
cherokee_admin_server_reply_get_sources (cherokee_handler_t *hdl,
                                         cherokee_dwriter_t *dwriter)
{
	cherokee_server_t *srv = HANDLER_SRV(hdl);

	cherokee_dwriter_list_open (dwriter);
	cherokee_avl_while (AVL_GENERIC(&srv->sources), sources_while, dwriter, NULL, NULL);
	cherokee_dwriter_list_close (dwriter);

	return ret_ok;
}

ret_t
cherokee_admin_server_reply_kill_source (cherokee_handler_t *hdl,
                                         cherokee_dwriter_t *dwriter,
                                         cherokee_buffer_t  *question)
{
	ret_t              ret;
	int                re;
	char              *begin;
	char              *end;
	char              *p;
	cuint_t            n;
	char               id[10];
	cherokee_source_t *source = NULL;
	cherokee_server_t *srv    = HANDLER_SRV(hdl);
	cherokee_buffer_t  match  = CHEROKEE_BUF_INIT;

	cherokee_buffer_fake_str (&match, "kill server.source ");

	/* Check the command
	 */
	if (strncasecmp (question->buf, match.buf, match.len)) {
		return ret_error;
	}
	begin = question->buf + match.len;
	end   = question->buf + question->len;

	/* Check the source to be killed
	 */
	n = 0;
	p = begin;
	while (CHEROKEE_CHAR_IS_DIGIT(*p) && (p < end) && (n < 10)) {
		id[n] = *p;
		p++;
		n++;
	}
	id[n] = '\0';

	if (unlikely ((n == 0) || (n == 10))) {
		cherokee_dwriter_dict_open (dwriter);
		cherokee_dwriter_cstring (dwriter, "source");
		cherokee_dwriter_cstring (dwriter, "invalid");
		cherokee_dwriter_dict_close (dwriter);
		return ret_ok;
	}

	/* Find it on the AVL tree
	 */
	ret = cherokee_avl_get_ptr (&srv->sources, id, (void **)&source);
	if (ret != ret_ok) {
		cherokee_dwriter_dict_open (dwriter);
		cherokee_dwriter_cstring (dwriter, "source");
		cherokee_dwriter_cstring (dwriter, "not found");
		cherokee_dwriter_dict_close (dwriter);
		return ret_ok;
	}

	if ((source != NULL) &&
	    ((source->type != source_interpreter) ||
	    ((source->type == source_interpreter) && (SOURCE_INT(source)->pid <= 1))))
	{
		cherokee_dwriter_dict_open (dwriter);
		cherokee_dwriter_cstring (dwriter, "source");
		cherokee_dwriter_cstring (dwriter, "nothing to kill");
		cherokee_dwriter_dict_close (dwriter);
		return ret_ok;
	}

	/* Kill the process
	 */
	if (getuid() == 0) {
		/* Looks like we can actually kill the process
		 */
		re = kill (SOURCE_INT(source)->pid, SIGTERM);

	} else {
		/* TODO: It should be the 'cherokee' supervisor (running as root)
		 * the one in charge of killing the process.
		 */
		re = kill (SOURCE_INT(source)->pid, SIGTERM);
	}

	if (re == 0) {
		cherokee_dwriter_dict_open (dwriter);
		cherokee_dwriter_cstring (dwriter, "source");
		cherokee_dwriter_cstring (dwriter, "killed");
		cherokee_dwriter_dict_close (dwriter);
		SOURCE_INT(source)->pid = -1;

	} else if (errno == ESRCH) {
		cherokee_dwriter_dict_open (dwriter);
		cherokee_dwriter_cstring (dwriter, "source");
		cherokee_dwriter_cstring (dwriter, "nothing to kill");
		cherokee_dwriter_dict_close (dwriter);
		SOURCE_INT(source)->pid = -1;

	} else if (errno == EPERM) {
		cherokee_dwriter_dict_open (dwriter);
		cherokee_dwriter_cstring (dwriter, "source");
		cherokee_dwriter_cstring (dwriter, "no permission");
		cherokee_dwriter_dict_close (dwriter);
		// need to keep the pid, its still there
	}

	return ret_ok;
}


ret_t
cherokee_admin_server_reply_restart (cherokee_handler_t *hdl,
                                     cherokee_dwriter_t *dwriter)
{
	ret_t ret;

	/* Initialise the restart
	 */
	ret = cherokee_server_handle_HUP (HANDLER_SRV (hdl));
	if (ret != ret_ok) {
		return ret_error;
	}

	/* Reply
	 */
	cherokee_dwriter_dict_open (dwriter);
	cherokee_dwriter_cstring (dwriter, "restart");
	cherokee_dwriter_bool (dwriter, ret == ret_ok);
	cherokee_dwriter_dict_close (dwriter);
	return ret_ok;
}
