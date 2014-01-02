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
#include "logger_custom.h"

#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "module.h"
#include "server.h"
#include "server-protected.h"
#include "header.h"
#include "header-protected.h"

/* Plug-in initialization
 */
PLUGIN_INFO_LOGGER_EASIEST_INIT (custom);


/* Global stuff
 */
static cherokee_buffer_t now;


/* The macros
 */
static ret_t
add_ip_remote (cherokee_template_t       *template,
               cherokee_template_token_t *token,
               cherokee_buffer_t         *output,
               void                      *param)
{
	cuint_t                prev_len;
	cherokee_connection_t *conn      = CONN(param);

	UNUSED (template);
	UNUSED (token);

	/* It has a X-Real-IP
	 */
	if (! cherokee_buffer_is_empty (&conn->logger_real_ip)) {
		cherokee_buffer_add_buffer (output, &conn->logger_real_ip);
		return ret_ok;
	}

	/* Render the IP string
	 */
	prev_len = output->len;

	cherokee_buffer_ensure_addlen (output, CHE_INET_ADDRSTRLEN);
	cherokee_socket_ntop (&conn->socket,
	                      (output->buf + output->len),
	                      (output->size - output->len) -1);

	output->len += strlen(output->buf + prev_len);
	return ret_ok;
}

static ret_t
add_ip_local (cherokee_template_t       *template,
              cherokee_template_token_t *token,
              cherokee_buffer_t         *output,
              void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if (! cherokee_buffer_is_empty (&conn->bind->ip)) {
		cherokee_buffer_add_buffer (output, &conn->bind->ip);
	} else {
		cherokee_buffer_add_str (output, "-");
	}

	return ret_ok;
}

static ret_t
add_status (cherokee_template_t       *template,
            cherokee_template_token_t *token,
            cherokee_buffer_t         *output,
            void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if (unlikely (conn->error_internal_code != http_unset)) {
		cherokee_buffer_add_long10 (output, conn->error_internal_code);
	} else {
		cherokee_buffer_add_ulong10 (output, conn->error_code);
	}

	return ret_ok;
}

static ret_t
add_transport (cherokee_template_t       *template,
               cherokee_template_token_t *token,
               cherokee_buffer_t         *output,
               void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if (conn->socket.is_tls) {
		cherokee_buffer_add_str (output, "https");
	} else {
		cherokee_buffer_add_str (output, "http");
	}

	return ret_ok;
}

static ret_t
add_protocol (cherokee_template_t       *template,
              cherokee_template_token_t *token,
              cherokee_buffer_t         *output,
              void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	switch (conn->header.version) {
	case http_version_11:
		cherokee_buffer_add_str (output, "HTTP/1.1");
		break;
	case http_version_10:
		cherokee_buffer_add_str (output, "HTTP/1.0");
		break;
	case http_version_09:
		cherokee_buffer_add_str (output, "HTTP/0.9");
		break;
	default:
		cherokee_buffer_add_str (output, "Unknown");
	}

	return ret_ok;
}

static ret_t
add_port_server (cherokee_template_t       *template,
                 cherokee_template_token_t *token,
                 cherokee_buffer_t         *output,
                 void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	cherokee_buffer_add_buffer (output, &conn->bind->server_port);
	return ret_ok;
}

static ret_t
add_query_string (cherokee_template_t       *template,
                  cherokee_template_token_t *token,
                  cherokee_buffer_t         *output,
                  void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if (! cherokee_buffer_is_empty(&conn->query_string)) {
		cherokee_buffer_add_buffer (output, &conn->query_string);
	} else {
		cherokee_buffer_add_str (output, "-");
	}

	return ret_ok;
}

static ret_t
add_request_first_line (cherokee_template_t       *template,
                        cherokee_template_token_t *token,
                        cherokee_buffer_t         *output,
                        void                      *param)
{
	char                  *p;
	char                  *end;
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	end = (conn->header.input_buffer->buf +
	       conn->header.input_buffer->len);

	p =  conn->header.input_buffer->buf;
	p += conn->header.request_off;

	while ((*p != CHR_CR) && (*p != CHR_LF) && (p < end))
		p++;

	cherokee_buffer_add (output,
	                     conn->header.input_buffer->buf,
	                     p - conn->header.input_buffer->buf);

	return ret_ok;
}

static ret_t
add_now (cherokee_template_t       *template,
         cherokee_template_token_t *token,
         cherokee_buffer_t         *output,
         void                      *param)
{
	UNUSED (template);
	UNUSED (token);
	UNUSED (param);

	return cherokee_buffer_add_buffer (output, &now);
}

static ret_t
add_time_secs (cherokee_template_t       *template,
               cherokee_template_token_t *token,
               cherokee_buffer_t         *output,
               void                      *param)
{
	UNUSED (template);
	UNUSED (token);
	UNUSED (param);

	return cherokee_buffer_add_long10 (output, cherokee_bogonow_now);
}

static ret_t
add_time_msecs (cherokee_template_t       *template,
                cherokee_template_token_t *token,
                cherokee_buffer_t         *output,
                void                      *param)
{
	UNUSED (template);
	UNUSED (token);
	UNUSED (param);

	return cherokee_buffer_add_ullong10 (output, cherokee_bogonow_msec);
}

static ret_t
add_user_remote (cherokee_template_t       *template,
                 cherokee_template_token_t *token,
                 cherokee_buffer_t         *output,
                 void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if ((conn->validator) &&
	    (! cherokee_buffer_is_empty (&conn->validator->user)))
	{
		cherokee_buffer_add_buffer (output, &conn->validator->user);
	} else {
		cherokee_buffer_add_str (output, "-");
	}

	return ret_ok;
}

static ret_t
add_request (cherokee_template_t       *template,
             cherokee_template_token_t *token,
             cherokee_buffer_t         *output,
             void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	cherokee_buffer_add_buffer (output, &conn->request);
	return ret_ok;
}

static ret_t
add_request_original (cherokee_template_t       *template,
                      cherokee_template_token_t *token,
                      cherokee_buffer_t         *output,
                      void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if (cherokee_buffer_is_empty (&conn->request_original)) {
		cherokee_buffer_add_buffer (output, &conn->request);
	} else  {
		cherokee_buffer_add_buffer (output, &conn->request_original);
	}

	return ret_ok;
}

static ret_t
add_vserver_name (cherokee_template_t       *template,
                  cherokee_template_token_t *token,
                  cherokee_buffer_t         *output,
                  void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	cherokee_buffer_add_buffer (output, &CONN_VSRV(conn)->name);
	return ret_ok;
}

static ret_t
add_vserver_name_req (cherokee_template_t       *template,
                      cherokee_template_token_t *token,
                      cherokee_buffer_t         *output,
                      void                      *param)
{
	ret_t                  ret;
	char                  *colon;
	char                  *header     = NULL;
	cuint_t                header_len = 0;
	cherokee_connection_t *conn       = CONN(param);

	UNUSED (template);
	UNUSED (token);

	/* Log the 'Host:' header
	 */
	ret = cherokee_header_get_known (&conn->header, header_host, &header, &header_len);
	if ((ret == ret_ok) && (header)) {
		colon = strchr (header, ':');
		if (colon) {
			cherokee_buffer_add (output, header, colon - header);
		} else {
			cherokee_buffer_add (output, header, header_len);
		}
		return ret_ok;
	}

	/* Plan B: Use the virtual server nick
	 */
	cherokee_buffer_add_buffer (output, &CONN_VSRV(conn)->name);
	return ret_ok;
}

static ret_t
add_response_size (cherokee_template_t       *template,
                   cherokee_template_token_t *token,
                   cherokee_buffer_t         *output,
                   void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	cherokee_buffer_add_ullong10 (output, conn->tx);
	return ret_ok;
}

static ret_t
add_http_host  (cherokee_template_t       *template,
                cherokee_template_token_t *token,
                cherokee_buffer_t         *output,
                void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if (! cherokee_buffer_is_empty (&conn->host)) {
		cherokee_buffer_add_buffer (output, &conn->host);
	} else {
		cherokee_buffer_add_char (output, '-');
	}

	return ret_ok;
}

static ret_t
add_http_referer  (cherokee_template_t       *template,
                   cherokee_template_token_t *token,
                   cherokee_buffer_t         *output,
                   void                      *param)
{
	ret_t                  ret;
	char                  *referer     = NULL;
	cuint_t                referer_len = 0;
	cherokee_connection_t *conn        = CONN(param);

	UNUSED (template);
	UNUSED (token);

	ret = cherokee_header_get_known (&conn->header, header_referer, &referer, &referer_len);
	if (ret != ret_ok) {
		cherokee_buffer_add_char (output, '-');
		return ret_ok;
	}

	cherokee_buffer_add (output, referer, referer_len);
	return ret_ok;
}

static ret_t
add_http_user_agent  (cherokee_template_t       *template,
                      cherokee_template_token_t *token,
                      cherokee_buffer_t         *output,
                      void                      *param)
{
	ret_t                  ret;
	char                  *user_agent     = NULL;
	cuint_t                user_agent_len = 0;
	cherokee_connection_t *conn           = CONN(param);

	UNUSED (template);
	UNUSED (token);

	ret = cherokee_header_get_known (&conn->header, header_user_agent, &user_agent, &user_agent_len);
	if (ret != ret_ok) {
		cherokee_buffer_add_char (output, '-');
		return ret_ok;
	}

	cherokee_buffer_add (output, user_agent, user_agent_len);
	return ret_ok;
}

static ret_t
add_http_cookie  (cherokee_template_t       *template,
                  cherokee_template_token_t *token,
                  cherokee_buffer_t         *output,
                  void                      *param)
{
	ret_t                  ret;
	char                  *cookie     = NULL;
	cuint_t                cookie_len = 0;
	cherokee_connection_t *conn       = CONN(param);

	UNUSED (template);
	UNUSED (token);

	ret = cherokee_header_get_known (&conn->header, header_cookie, &cookie, &cookie_len);
	if (ret != ret_ok) {
		cherokee_buffer_add_char (output, '-');
		return ret_ok;
	}

	cherokee_buffer_add (output, cookie, cookie_len);
	return ret_ok;
}


static ret_t
_set_template (cherokee_logger_custom_t *logger,
	       cherokee_template_t      *template)
{
	ret_t ret;
	const struct {
		const char *name;
		void       *func;
	} *p, macros[] = {
		{"ip_remote",          add_ip_remote},
		{"ip_local",           add_ip_local},
		{"protocol",           add_protocol},
		{"transport",          add_transport},
		{"port_server",        add_port_server},
		{"query_string",       add_query_string},
		{"request_first_line", add_request_first_line},
		{"status",             add_status},
		{"now",                add_now},
		{"time_secs",          add_time_secs},
		{"time_msecs",         add_time_msecs},
		{"user_remote",        add_user_remote},
		{"request",            add_request},
		{"request_original",   add_request_original},
		{"vserver_name",       add_vserver_name},
		{"vserver_name_req",   add_vserver_name_req},
		{"response_size",      add_response_size},
		{"http_host",          add_http_host},
		{"http_referrer",      add_http_referer},
		{"http_user_agent",    add_http_user_agent},
		{"http_cookie",        add_http_cookie},
		{NULL, NULL}
	};

	for (p = macros; p->name; p++) {
		ret = cherokee_template_set_token (template, p->name,
		                                   (cherokee_tem_repl_func_t) p->func,
		                                   logger, NULL);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}
	}

	return ret_ok;
}


static ret_t
_init_template (cherokee_logger_custom_t *logger,
                cherokee_template_t      *template,
                cherokee_config_node_t   *config,
                const char               *key_config)
{
	ret_t              ret;
	cherokee_buffer_t *tmp;

	ret = cherokee_template_init (template);
	if (ret != ret_ok)
		return ret;

	ret = _set_template (logger, template);
	if (ret != ret_ok)
		return ret;

	ret = cherokee_config_node_read (config, key_config, &tmp);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_LOGGER_CUSTOM_NO_TEMPLATE, key_config);
		return ret_error;
	}

	ret = cherokee_template_parse (template, tmp);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_LOGGER_CUSTOM_TEMPLATE, tmp->buf);
		return ret_error;
	}

	return ret_ok;
}


static void
bogotime_callback (void *param)
{
	struct tm                *pnow_tm;
	cherokee_logger_custom_t *logger   = LOG_CUSTOM(param);

	/* Choose between local and universal time
	 */
	if (LOGGER(logger)->utc_time) {
		pnow_tm = &cherokee_bogonow_tmgmt;
	} else {
		pnow_tm = &cherokee_bogonow_tmloc;
	}

	/* Render the string
	 */
	cherokee_buffer_clean  (&now);
	cherokee_buffer_add_va (&now,
	                        "%02d/%s/%d:%02d:%02d:%02d %c%02d%02d",
	                        pnow_tm->tm_mday,
	                        month[pnow_tm->tm_mon],
	                        1900 + pnow_tm->tm_year,
	                        pnow_tm->tm_hour,
	                        pnow_tm->tm_min,
	                        pnow_tm->tm_sec,
	                        (cherokee_bogonow_tzloc < 0) ? '-' : '+',
	                        (int) (abs(cherokee_bogonow_tzloc) / 60),
	                        (int) (abs(cherokee_bogonow_tzloc) % 60));
}


ret_t
cherokee_logger_custom_new (cherokee_logger_t         **logger,
                            cherokee_virtual_server_t  *vsrv,
                            cherokee_config_node_t     *config)
{
	ret_t                   ret;
	static int              callback_init = 0;
	cherokee_config_node_t *subconf;
	CHEROKEE_NEW_STRUCT (n, logger_custom);

	/* Init the base class object
	 */
	cherokee_logger_init_base (LOGGER(n), PLUGIN_INFO_PTR(custom), config);

	MODULE(n)->init         = (logger_func_init_t) cherokee_logger_custom_init;
	MODULE(n)->free         = (logger_func_free_t) cherokee_logger_custom_free;

	LOGGER(n)->flush        = (logger_func_flush_t) cherokee_logger_custom_flush;
	LOGGER(n)->reopen       = (logger_func_reopen_t) cherokee_logger_custom_reopen;
	LOGGER(n)->write_access = (logger_func_write_access_t) cherokee_logger_custom_write_access;

	/* Init properties
	 */
	ret = cherokee_config_node_get (config, "access", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_LOGGER_NO_KEY, "access");
		goto error;
	}
	ret = cherokee_server_get_log_writer (VSERVER_SRV(vsrv), subconf, &n->writer_access);
	if (ret != ret_ok) {
		goto error;
	}

	/* Template
	 */
	ret = _init_template (n, &n->template_conn, config, "access_template");
	if (ret != ret_ok) {
		goto error;
	}

	/* Callback init
	 */
	if (callback_init == 0) {
		cherokee_buffer_init (&now);
		cherokee_bogotime_add_callback (bogotime_callback, n, 1);
	}

	/* Return the object
	 */
	*logger = LOGGER(n);
	return ret_ok;

error:
	cherokee_logger_free (LOGGER(n));
	return ret_error;
}

ret_t
cherokee_logger_custom_init (cherokee_logger_custom_t *logger)
{
	ret_t ret;

	ret = cherokee_logger_writer_open (logger->writer_access);
	if (ret != ret_ok) {
		return ret;
	}

	return ret_ok;
}

ret_t
cherokee_logger_custom_free (cherokee_logger_custom_t *logger)
{
	cherokee_buffer_mrproper (&now);

	cherokee_template_mrproper (&logger->template_conn);
	return ret_ok;
}

ret_t
cherokee_logger_custom_flush (cherokee_logger_custom_t *logger)
{
	return cherokee_logger_writer_flush (logger->writer_access, false);
}

ret_t
cherokee_logger_custom_reopen (cherokee_logger_custom_t *logger)
{
	ret_t ret;

	ret = cherokee_logger_writer_reopen (logger->writer_access);
	if (ret != ret_ok) {
		return ret;
	}

	return ret_ok;
}

ret_t
cherokee_logger_custom_write_access (cherokee_logger_custom_t *logger,
                                     cherokee_connection_t    *conn)
{
	ret_t              ret;
	cherokee_buffer_t *log;

	/* Get the buffer
	 */
	cherokee_logger_writer_get_buf (logger->writer_access, &log);

	/* Render the template
	 */
	ret = cherokee_template_render (&logger->template_conn, log, conn);
	if (unlikely (ret != ret_ok)) {
		goto error;
	}

	cherokee_buffer_add_char (log, '\n');

	/* Flush buffer if full
	 */
	if (log->len < logger->writer_access->max_bufsize)
		goto ok;

	ret = cherokee_logger_writer_flush (logger->writer_access, true);
	if (unlikely (ret != ret_ok)) {
		goto error;
	}

ok:
	cherokee_logger_writer_release_buf (logger->writer_access);
	return ret_ok;

error:
	cherokee_logger_writer_release_buf (logger->writer_access);
	return ret_error;
}


ret_t
cherokee_logger_custom_write_string (cherokee_logger_custom_t *logger,
                                     const char               *string)
{
	ret_t              ret;
	cherokee_buffer_t *log;

	/* Get the buffer
	 */
	cherokee_logger_writer_get_buf (logger->writer_access, &log);

	ret = cherokee_buffer_add (log, string, strlen(string));
	if (unlikely (ret != ret_ok)) {
		goto error;
	}

	/* Flush buffer if full
	 */
	if (log->len < logger->writer_access->max_bufsize) {
		goto ok;
	}

	ret = cherokee_logger_writer_flush (logger->writer_access, true);
	if (unlikely (ret != ret_ok)) {
		goto error;
	}

ok:
	cherokee_logger_writer_release_buf (logger->writer_access);
	return ret_ok;

error:
	cherokee_logger_writer_release_buf (logger->writer_access);
	return ret_error;
}
