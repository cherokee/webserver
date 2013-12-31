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
#include "handler_secdownload.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "handler,secdownload"

ret_t
cherokee_handler_secdownload_props_free (cherokee_handler_secdownload_props_t *props)
{
	if (props->props_file != NULL) {
		cherokee_handler_file_props_free (props->props_file);
		props->props_file = NULL;
	}

	cherokee_buffer_mrproper (&props->secret);

	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


ret_t
cherokee_handler_secdownload_configure (cherokee_config_node_t   *conf,
                                        cherokee_server_t        *srv,
                                        cherokee_module_props_t **_props)
{
	ret_t                                 ret;
	cherokee_handler_secdownload_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_secdownload_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
		     MODULE_PROPS_FREE(cherokee_handler_secdownload_props_free));

		cherokee_buffer_init (&n->secret);
		n->timeout = 60;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_SECDOWN(*_props);

	/* Parse 'file' parameters
	 */
	props->props_file = NULL;
	ret = cherokee_handler_file_configure (conf, srv, (cherokee_module_props_t **)&props->props_file);
	if ((ret != ret_ok) && (ret != ret_deny))
		return ret;

	/* Properties
	 */
	ret = cherokee_config_node_copy (conf, "secret", &props->secret);
	if (ret != ret_ok) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_HANDLER_SECDOWN_SECRET);
		return ret_error;
	}

	cherokee_config_node_read_int (conf, "timeout", (int*)&props->timeout);

	return ret_ok;
}


static int
check_hex (char *p, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (!((p[i] >= '0' && p[i] <= '9') ||
		      (p[i] >= 'a' && p[i] <= 'f') ||
		      (p[i] >= 'A' && p[i] <= 'F')))
		{
			return 1;
		}
	}

	return 0;
}

static int
get_time (char *p)
{
	int    i;
	time_t time = 0;

	for (i = 0; i < 8; i++) {
		time = (time << 4) + hex2dec_tab[(int)p[i]];
	}

	return time;
}


ret_t
cherokee_handler_secdownload_new (cherokee_handler_t     **hdl,
                                  void                    *cnt,
                                  cherokee_module_props_t *props)
{
	int                    re;
	ret_t                  ret;
	char                  *p;
	cuint_t                path_len;
	time_t                 time_url;
	char                  *time_s;
	cherokee_buffer_t      md5      = CHEROKEE_BUF_INIT;
	cherokee_connection_t *conn     = CONN(cnt);

	TRACE(ENTRIES, "Analyzing request '%s'\n", conn->request.buf);

	/* Sanity check
	 */
	if (conn->request.len <= 1 + 32 + 2) {
		TRACE(ENTRIES, "Malformed URL. Too short: len=%d.\n", conn->request.len);
		conn->error_code = http_not_found;
		return ret_error;
	}

	if (conn->request.buf[0] != '/') {
		TRACE(ENTRIES, "Malformed URL: %s\n", "Not slash (1)");
		conn->error_code = http_not_found;
		return ret_error;
	}

	p = conn->request.buf + 1;
	if (check_hex (p, 32)) {
		TRACE(ENTRIES, "Malformed URL: %s\n", "No MD5");
		conn->error_code = http_not_found;
		return ret_error;
	}
	p += 32;

	if (*p != '/') {
		TRACE(ENTRIES, "Malformed URL: %s\n", "Not slash (2)");
		conn->error_code = http_not_found;
		return ret_error;
	}
	p += 1;
	time_s = p;

	if (check_hex (p, 8)) {
		TRACE(ENTRIES, "Malformed URL: %s\n", "No MD5 (2)");
		conn->error_code = http_not_found;
		return ret_error;
	}
	p += 8;

	/* Check the time
	 */
	time_url = get_time (time_s);
	if ((cherokee_bogonow_now - time_url) > (int)PROP_SECDOWN(props)->timeout) {
		TRACE(ENTRIES, "Time out: %d (now=%d)\n", time_url, cherokee_bogonow_now);
		conn->error_code = http_gone;
		return ret_error;
	}

	/* Check the MD5
	 * [secret][path][hex(time)]
	 */
	path_len = (conn->request.buf + conn->request.len) - p;

	cherokee_buffer_add_buffer (&md5, &PROP_SECDOWN(props)->secret);
	cherokee_buffer_add        (&md5, p, path_len);
	cherokee_buffer_add        (&md5, time_s, 8);

	cherokee_buffer_encode_md5_digest (&md5);

	re = strncasecmp (md5.buf, &conn->request.buf[1], 32);
	if (re != 0) {
#ifdef TRACE_ENABLED
		if (cherokee_trace_is_tracing()) {
			cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

			cherokee_buffer_add_str    (&tmp, "secret='");
			cherokee_buffer_add_buffer (&tmp, &PROP_SECDOWN(props)->secret);
			cherokee_buffer_add_str    (&tmp, "', path='");
			cherokee_buffer_add        (&tmp, p, path_len);
			cherokee_buffer_add_str    (&tmp, "', time='");
			cherokee_buffer_add        (&tmp, time_s, 8);
			cherokee_buffer_add_str    (&tmp, "'");

			TRACE(ENTRIES, "MD5 (%s) didn't match (%s)\n", md5.buf, tmp.buf);
			cherokee_buffer_mrproper(&tmp);
		}
#endif

		cherokee_buffer_mrproper(&md5);
		conn->error_code = http_access_denied;
		return ret_error;
	}

	cherokee_buffer_mrproper (&md5);

	/* At this point the request has been validated
	 */
	if (cherokee_buffer_is_empty (&conn->request_original)) {
		cherokee_buffer_add_buffer (&conn->request_original, &conn->request);
		cherokee_buffer_add_buffer (&conn->query_string_original, &conn->query_string);
	}

	cherokee_buffer_clean (&conn->request);
	cherokee_buffer_add   (&conn->request, p, path_len);

	/* Instance the File handler
	 */
	ret = cherokee_handler_file_new (hdl, cnt, MODULE_PROPS(PROP_SECDOWN(props)->props_file));
	if (ret != ret_ok)
		return ret_ok;

	return ret_ok;
}


/* Library init function
 */
static cherokee_boolean_t _secdownload_is_init = false;

void
PLUGIN_INIT_NAME(secdownload) (cherokee_plugin_loader_t *loader)
{
	if (_secdownload_is_init) return;
	_secdownload_is_init = true;

	/* Load the dependences
	 */
	cherokee_plugin_loader_load (loader, "file");
}

PLUGIN_INFO_HANDLER_EASY_INIT (secdownload, http_all_methods);
