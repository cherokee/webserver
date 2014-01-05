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

#include "handler_post_report.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "thread.h"
#include "plugin_loader.h"
#include "util.h"

#define ENTRIES "handler,post,track"


/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (post_report, http_get);

/* Methods implementation
 */
static ret_t
props_free (cherokee_handler_post_report_props_t *props)
{
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}

ret_t
cherokee_handler_post_report_configure (cherokee_config_node_t   *conf,
                                        cherokee_server_t        *srv,
                                        cherokee_module_props_t **_props)
{
	ret_t                                 ret;
	cherokee_list_t                      *i;
	cherokee_handler_post_report_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_post_report_props);

		cherokee_module_props_init_base (MODULE_PROPS(n),
		                                 MODULE_PROPS_FREE(props_free));
		n->lang = dwriter_json;
		*_props = MODULE_PROPS(n);
	}

	props = PROP_POST_REPORT(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		ret = cherokee_dwriter_lang_to_type (&subconf->val, &props->lang);
		if (ret != ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_HANDLER_POST_REPORT_LANG, subconf->val.buf);
			return ret_error;
		}
	}

	return ret_ok;
}

ret_t
cherokee_handler_post_report_new  (cherokee_handler_t      **hdl,
                                   cherokee_connection_t    *cnt,
                                   cherokee_module_props_t  *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_post_report);

	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(post_report));

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_post_report_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_post_report_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_post_report_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_post_report_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support = hsupport_nothing;

	/* Init
	 */
	ret = cherokee_buffer_init (&n->buffer);
	if (unlikely(ret != ret_ok)) {
		return ret;
	}

	ret = cherokee_dwriter_init (&n->writer, &CONN_THREAD(cnt)->tmp_buf1);
	if (unlikely(ret != ret_ok)) {
		return ret;
	}

	n->writer.pretty = true;
	n->writer.lang   = PROP_POST_REPORT(props)->lang;

	cherokee_dwriter_set_buffer (&n->writer, &n->buffer);

	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_post_report_free (cherokee_handler_post_report_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->buffer);
	cherokee_dwriter_mrproper (&hdl->writer);
	return ret_ok;
}

static ret_t
_figure_x_progress_id (cherokee_connection_t  *conn,
                       cherokee_buffer_t     **id)
{
	ret_t ret;

	ret = cherokee_connection_parse_args (conn);
	if (ret != ret_ok) {
		return ret_error;
	}

	ret = cherokee_avl_get_ptr (conn->arguments, "X-Progress-ID", (void **)id);
	if ((ret == ret_ok) && (*id != NULL) && ((*id)->len > 0)) {
		return ret_ok;
	}

	return ret_error;
}

ret_t
cherokee_handler_post_report_init (cherokee_handler_post_report_t *hdl)
{
	ret_t                  ret;
	const char            *state;
	off_t                  size      = 0;
	off_t                  received  = 0;
	cherokee_buffer_t     *id        = NULL;
	cherokee_server_t     *srv       = HANDLER_SRV(hdl);
	cherokee_connection_t *conn      = HANDLER_CONN(hdl);

	/* POST tracking is disabled
	 */
	if (srv->post_track == NULL) {
		cherokee_dwriter_dict_open (&hdl->writer);
		cherokee_dwriter_cstring (&hdl->writer, "error");
		cherokee_dwriter_cstring (&hdl->writer, "Tracking is disabled");
		cherokee_dwriter_dict_close (&hdl->writer);

		TRACE (ENTRIES, "Post report: '%s'\n", "disabled");
		return ret_ok;
	}

	/* Figure X-Progress-ID
	 */
	ret = _figure_x_progress_id (conn, &id);
	if (ret != ret_ok) {
		cherokee_dwriter_dict_open (&hdl->writer);
		cherokee_dwriter_cstring (&hdl->writer, "error");
		cherokee_dwriter_cstring (&hdl->writer, "Could not read the X-Progress-ID parameter");
		cherokee_dwriter_dict_close (&hdl->writer);

		TRACE (ENTRIES, "Post report: '%s'\n", "No X-Progress-ID");
		return ret_ok;
	}

	/* Check in the POST tracker
	 */
	ret = cherokee_generic_post_track_get (srv->post_track, id,
	                                       &state, &size, &received);
	if (ret != ret_ok) {
		cherokee_dwriter_dict_open (&hdl->writer);
		cherokee_dwriter_cstring (&hdl->writer, "error");
		cherokee_dwriter_string (&hdl->writer, state, strlen(state));
		cherokee_dwriter_dict_close (&hdl->writer);

		TRACE (ENTRIES, "Post report: '%s'\n", "Could not get info");
		return ret_ok;
	}

	cherokee_dwriter_dict_open (&hdl->writer);
	cherokee_dwriter_cstring (&hdl->writer, "state");
	cherokee_dwriter_string  (&hdl->writer, state, strlen(state));
	cherokee_dwriter_cstring (&hdl->writer, "size");
	cherokee_dwriter_unsigned (&hdl->writer, size);
	cherokee_dwriter_cstring (&hdl->writer, "received");
	cherokee_dwriter_unsigned (&hdl->writer, received);
	cherokee_dwriter_dict_close (&hdl->writer);

	TRACE (ENTRIES, "Post report: %llu of %llu\n", received, size);
	return ret_ok;
}


ret_t
cherokee_handler_post_report_add_headers (cherokee_handler_post_report_t *hdl,
                                          cherokee_buffer_t              *buffer)
{
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	if (cherokee_connection_should_include_length(conn)) {
		HANDLER(hdl)->support |= hsupport_length;
		cherokee_buffer_add_va (buffer, "Content-Length: %d"CRLF, hdl->buffer.len);
	}

	cherokee_buffer_add_str (buffer, "Content-Type: application/json" CRLF);
	return ret_ok;
}


ret_t
cherokee_handler_post_report_step (cherokee_handler_post_report_t *hdl,
                                   cherokee_buffer_t              *buffer)
{
	ret_t ret;

	ret = cherokee_buffer_add_buffer (buffer, &hdl->buffer);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	return ret_eof_have_data;
}
