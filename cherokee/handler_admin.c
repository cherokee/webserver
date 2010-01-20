/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2010 Alvaro Lopez Ortega
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
#include "handler_admin.h"
#include "connection.h"
#include "connection-protected.h"
#include "server.h"
#include "server-protected.h"
#include "admin_server.h"
#include "util.h"

#define ERR_STR(x)

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (admin, http_get | http_post);


/* Methods implementation
 */
ret_t
cherokee_handler_admin_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	UNUSED(conf);
	UNUSED(srv);
	UNUSED(_props);

	return ret_ok;
}


ret_t
cherokee_handler_admin_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_admin);

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(admin));

	MODULE(n)->init         = (module_func_init_t) cherokee_handler_admin_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_admin_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_admin_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_admin_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_nothing;

	cherokee_buffer_init (&n->reply);

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_admin_free (cherokee_handler_admin_t *ahdl)
{
	cherokee_buffer_mrproper (&ahdl->reply);
	return ret_ok;
}


static ret_t
process_request_line (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *line)
{
#define COMP(str,sub) strncmp(str, sub, sizeof(sub)-1)

	if (!COMP (line->buf, "get server.port_tls"))
		return cherokee_admin_server_reply_get_port_tls (ahdl, line, &ahdl->reply);
	else if (!COMP (line->buf, "set server.port_tls"))
		return cherokee_admin_server_reply_set_port_tls (ahdl, line, &ahdl->reply);

	else if (!COMP (line->buf, "get server.port"))
		return cherokee_admin_server_reply_get_port (ahdl, line, &ahdl->reply);
	else if (!COMP (line->buf, "set server.port"))
		return cherokee_admin_server_reply_set_port (ahdl, line, &ahdl->reply);

	else if (!COMP (line->buf, "get server.rx"))
		return cherokee_admin_server_reply_get_rx (ahdl, line, &ahdl->reply);
	else if (!COMP (line->buf, "get server.tx"))
		return cherokee_admin_server_reply_get_tx (ahdl, line, &ahdl->reply);

	else if (!COMP (line->buf, "get server.connections"))
		return cherokee_admin_server_reply_get_connections (ahdl, line, &ahdl->reply);

	else if (!COMP (line->buf, "del server.connection"))
		return cherokee_admin_server_reply_del_connection (ahdl, line, &ahdl->reply);

	else if (!COMP (line->buf, "get server.thread_num"))
		return cherokee_admin_server_reply_get_thread_num (ahdl, line, &ahdl->reply);

	else if (!COMP (line->buf, "set server.backup_mode"))
		return cherokee_admin_server_reply_set_backup_mode (ahdl, line, &ahdl->reply);

	else if (!COMP (line->buf, "set server.trace"))
		return cherokee_admin_server_reply_set_trace (ahdl, line, &ahdl->reply);
	else if (!COMP (line->buf, "get server.trace"))
		return cherokee_admin_server_reply_get_trace (ahdl, line, &ahdl->reply);

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
cherokee_handler_admin_init (cherokee_handler_admin_t *ahdl)
{
	ret_t                    ret;
	char                    *tmp;
	cherokee_buffer_t        post = CHEROKEE_BUF_INIT;
	cherokee_buffer_t        line = CHEROKEE_BUF_INIT;
	cherokee_connection_t   *conn = HANDLER_CONN(ahdl);

	/* Check for the post info
	 */
	if (conn->post.len <= 0) {
		conn->error_code = http_bad_request;
		return ret_error;
	}

	/* Process line per line
	 */
	do {
		ret = cherokee_post_read (&conn->post, &conn->socket, &post);
		if (unlikely (ret == ret_error)) {
			conn->error_code = http_bad_request;
			return ret_error;
		}
	} while (! cherokee_post_read_finished (&conn->post));

	for (tmp = post.buf;;) {
		char *end1 = strchr (tmp, CHR_LF);
		char *end2 = strchr (tmp, CHR_CR);
		char *end  = cherokee_min_str (end1, end2);

		if (end == NULL) break;
		if (end - tmp < 2) break;

		/* Copy current line and go to the next one
		 */
		cherokee_buffer_add (&line, tmp, end - tmp);
		while ((*end == CHR_CR) || (*end == CHR_LF)) end++;
		tmp = end;

		/* Process current line
		 */
		ret = process_request_line (ahdl, &line);
		if (ret == ret_error) {
			conn->error_code = http_bad_request;
			ret = ret_error;
			goto go_out;
		}

		/* Clean up for the next iteration
		 */
		cherokee_buffer_clean (&line);
	}

go_out:
	cherokee_buffer_mrproper (&post);
	cherokee_buffer_mrproper (&line);
	return ret;
}


ret_t
cherokee_handler_admin_step (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_buffer (buffer, &ahdl->reply);
	return ret_eof_have_data;
}


ret_t
cherokee_handler_admin_add_headers (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *buffer)
{
	cherokee_connection_t *conn = HANDLER_CONN(ahdl);

	if (cherokee_connection_should_include_length(conn)) {
		HANDLER(ahdl)->support = hsupport_length;
		cherokee_buffer_add_va (buffer, "Content-Length: %lu" CRLF, ahdl->reply.len);
	}

	return ret_ok;
}
