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
#include "handler_admin.h"
#include "connection.h"
#include "connection-protected.h"
#include "server.h"
#include "server-protected.h"
#include "admin_server.h"
#include "util.h"

#define ENTRIES "handler,admin"

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (admin, http_post | http_purge);


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
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_admin_add_headers;
	HANDLER(n)->read_post   = (handler_func_read_post_t) cherokee_handler_admin_read_post;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_admin_step;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_nothing;

	cherokee_buffer_init (&n->reply);

	/* Configure the data writer object
	 */
	cherokee_dwriter_init       (&n->dwriter, THREAD_TMP_BUF1(CONN_THREAD(cnt)));
	cherokee_dwriter_set_buffer (&n->dwriter, &n->reply);

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_admin_free (cherokee_handler_admin_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->reply);
	cherokee_dwriter_mrproper (&hdl->dwriter);

	return ret_ok;
}


static ret_t
process_request_line (cherokee_handler_admin_t *hdl, cherokee_buffer_t *line)
{
#define COMP(str,sub) (strncmp(str, sub, sizeof(sub)-1) == 0)

	if (COMP (line->buf, "get server.ports")) {
		return cherokee_admin_server_reply_get_ports (HANDLER(hdl), &hdl->dwriter);
	} else if (COMP (line->buf, "get server.traffic")) {
		return cherokee_admin_server_reply_get_traffic (HANDLER(hdl), &hdl->dwriter);
	} else if (COMP (line->buf, "get server.thread_num")) {
		return cherokee_admin_server_reply_get_thread_num (HANDLER(hdl), &hdl->dwriter);

	} else if (COMP (line->buf, "get server.trace")) {
		return cherokee_admin_server_reply_get_trace (HANDLER(hdl), &hdl->dwriter);
	} else if (COMP (line->buf, "set server.trace")) {
		return cherokee_admin_server_reply_set_trace (HANDLER(hdl), &hdl->dwriter, line);

	} else if (COMP (line->buf, "get server.sources")) {
		return cherokee_admin_server_reply_get_sources (HANDLER(hdl), &hdl->dwriter);
	} else if (COMP (line->buf, "kill server.source")) {
		return cherokee_admin_server_reply_kill_source (HANDLER(hdl), &hdl->dwriter, line);

	} else if (COMP (line->buf, "set server.backup_mode")) {
		return cherokee_admin_server_reply_set_backup_mode (HANDLER(hdl), &hdl->dwriter, line);

	} else if (COMP (line->buf, "get server.connections")) {
		return cherokee_admin_server_reply_get_conns (HANDLER(hdl), &hdl->dwriter);
	} else if (COMP (line->buf, "close server.connection")) {
		return cherokee_admin_server_reply_close_conn (HANDLER(hdl), &hdl->dwriter, line);

	} else if (COMP (line->buf, "restart")) {
		return cherokee_admin_server_reply_restart (HANDLER(hdl), &hdl->dwriter);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


static ret_t
front_line_cache_purge (cherokee_handler_admin_t *hdl)
{
	ret_t                      ret;
	cherokee_connection_t     *conn = HANDLER_CONN(hdl);
	cherokee_virtual_server_t *vsrv = HANDLER_VSRV(hdl);

	/* FLCache not active in the Virtual Server
	 */
	if (vsrv->flcache == NULL) {
		conn->error_code = http_not_found;
		return ret_error;
	}

	/* Remove the cache objects
	 */
	ret = cherokee_flcache_purge_path (vsrv->flcache, &conn->request);

	switch (ret) {
	case ret_ok:
		cherokee_dwriter_cstring (&hdl->dwriter, "ok");
		return ret_ok;
	case ret_not_found:
		cherokee_dwriter_cstring (&hdl->dwriter, "not found");
		conn->error_code = http_not_found;
		return ret_error;
	default:
		cherokee_dwriter_cstring (&hdl->dwriter, "error");
		conn->error_code = http_internal_error;
		return ret_error;

	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
cherokee_handler_admin_init (cherokee_handler_admin_t *hdl)
{
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

#define finishes_by(s) \
	((conn->request.len > sizeof(s)-1) && \
	 (!strncmp (conn->request.buf + conn->request.len - (sizeof(s)-1), s, sizeof(s)-1)))

	if (finishes_by ("/py")) {
		hdl->dwriter.lang = dwriter_python;
	} else if (finishes_by ("/js")) {
		hdl->dwriter.lang = dwriter_json;
	} else if (finishes_by ("/php")) {
		hdl->dwriter.lang = dwriter_php;
	} else if (finishes_by ("/ruby")) {
		hdl->dwriter.lang = dwriter_ruby;
	}

	/* Front-Line Cache's PURGE
	 */
	if (conn->header.method == http_purge) {
		return front_line_cache_purge (hdl);
	}

#undef finishes_by
	return ret_ok;
}


ret_t
cherokee_handler_admin_read_post (cherokee_handler_admin_t *hdl)
{
	int                      re;
	ret_t                    ret;
	char                    *tmp;
	cherokee_buffer_t        post = CHEROKEE_BUF_INIT;
	cherokee_buffer_t        line = CHEROKEE_BUF_INIT;
	cherokee_connection_t   *conn = HANDLER_CONN(hdl);

	/* Check for the post info
	 */
	if (! conn->post.has_info) {
		conn->error_code = http_bad_request;
		return ret_error;
	}

	/* Process line per line
	 */
	ret = cherokee_post_read (&conn->post, &conn->socket, &post);
	switch (ret) {
	case ret_ok:
	case ret_eagain:
		break;
	default:
		conn->error_code = http_bad_request;
		return ret_error;
	}

	/* Parse
	 */
	TRACE (ENTRIES, "Post contains: '%s'\n", post.buf);

	cherokee_dwriter_list_open (&hdl->dwriter);

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
		ret = process_request_line (hdl, &line);
		if (ret == ret_error) {
			conn->error_code = http_bad_request;
			ret = ret_error;
			goto exit2;
		}

		/* Clean up for the next iteration
		 */
		cherokee_buffer_clean (&line);
	}

	cherokee_dwriter_list_close (&hdl->dwriter);

	/* There might be more POST to read
	 */
	re = cherokee_post_read_finished (&conn->post);
	ret = re ? ret_ok : ret_eagain;

exit2:
	cherokee_buffer_mrproper (&post);
	cherokee_buffer_mrproper (&line);
	return ret;
}


ret_t
cherokee_handler_admin_add_headers (cherokee_handler_admin_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* Regular request
	 */
	if (cherokee_connection_should_include_length(conn)) {
		HANDLER(hdl)->support = hsupport_length;
		cherokee_buffer_add_va (buffer, "Content-Length: %lu" CRLF, hdl->reply.len);
	}

	return ret_ok;
}


ret_t
cherokee_handler_admin_step (cherokee_handler_admin_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_buffer (buffer, &hdl->reply);
	return ret_eof_have_data;
}
