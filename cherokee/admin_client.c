/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "common-internal.h"

#include <string.h>
#include "admin_client.h"

#include "util.h"
#include "buffer.h"
#include "downloader_async.h"
#include "downloader-protected.h"

#define ENTRIES "admin,client"


struct cherokee_admin_client {
	cherokee_downloader_async_t *downloader;

	cherokee_buffer_t           *url_ref;
	cherokee_buffer_t            request;
	cherokee_buffer_t            reply;

	cherokee_post_t              post;
	cherokee_fdpoll_t           *poll_ref;
};

#define strcmp_begin(line,sub)       strncmp(line, sub, strlen(sub))
#define strcmp_begin_const(line,sub) strncmp(line, sub, sizeof(sub))


ret_t 
cherokee_admin_client_new (cherokee_admin_client_t **admin)
{
	CHEROKEE_NEW_STRUCT(n,admin_client);

	/* Init
	 */
	n->poll_ref = NULL;
	n->url_ref  = NULL;

	cherokee_post_init (&n->post);
	cherokee_downloader_async_new (&n->downloader);
	cherokee_buffer_init (&n->request);
	cherokee_buffer_init (&n->reply);

	/* Return the object
	 */
	*admin = n;
	return ret_ok;
}


ret_t 
cherokee_admin_client_free (cherokee_admin_client_t *admin)
{
	cherokee_buffer_mrproper (&admin->request);
	cherokee_buffer_mrproper (&admin->reply);

	cherokee_downloader_async_free (admin->downloader);

	free (admin);
	return ret_ok;
}


ret_t 
cherokee_admin_client_prepare (cherokee_admin_client_t *admin, 
			       cherokee_fdpoll_t       *poll,
			       cherokee_buffer_t       *url,
			       cherokee_buffer_t       *user,
			       cherokee_buffer_t       *pass)
{
	ret_t                  ret;
	cherokee_downloader_t *downloader = DOWNLOADER(admin->downloader);

	admin->poll_ref = poll;
	admin->url_ref  = url;

	TRACE(ENTRIES, "fdpoll=%p url=%p\n", poll, url);

	/* Sanity check
	 */
	if ((admin->url_ref == NULL) || 
	    (admin->poll_ref == NULL))
	{
		PRINT_ERROR_S ("ERROR: Internal error\n");
		return ret_error;
	}
	
	/* Set up the downloader object properties
	 */
	ret = cherokee_downloader_async_set_fdpoll (DOWNLOADER_ASYNC(downloader), admin->poll_ref);
	if (unlikely (ret != ret_ok)) return ret;
	
	ret = cherokee_downloader_set_url (downloader, admin->url_ref); 
	if (unlikely (ret != ret_ok)) return ret;

	ret = cherokee_downloader_set_keepalive (downloader, true);
	if (unlikely (ret != ret_ok)) return ret;

	/* Set the authentication data
	 */
	ret = cherokee_downloader_set_auth (downloader, user, pass);
	if (unlikely (ret != ret_ok)) return ret;	

	TRACE(ENTRIES, "Exists obj=%p\n", admin);
	return ret_ok;
}


ret_t 
cherokee_admin_client_connect (cherokee_admin_client_t *admin)
{
	return cherokee_downloader_async_connect (admin->downloader);
}

ret_t 
cherokee_admin_client_get_reply_code (cherokee_admin_client_t *admin, cherokee_http_t *code)
{
	return cherokee_downloader_get_reply_code (DOWNLOADER(admin->downloader), code);
}

ret_t 
cherokee_admin_client_reuse (cherokee_admin_client_t *admin)
{
	cherokee_downloader_reuse (DOWNLOADER(admin->downloader));

	cherokee_buffer_clean (&admin->request);
	cherokee_buffer_clean (&admin->reply);

	return ret_ok;
}


static ret_t
internal_step (cherokee_admin_client_t *admin)
{
	ret_t                  ret;
	cherokee_downloader_t *downloader = DOWNLOADER(admin->downloader);

	TRACE(ENTRIES, "Downloader phase=%d\n", downloader->phase);

	/* It's stepping
	 */
	ret = cherokee_downloader_async_step (admin->downloader);
	switch (ret) {
	case ret_eof:
	case ret_eof_have_data:
		return ret_ok;
	case ret_error:
	case ret_eagain:
		return ret;
	case ret_ok:
		return ret_eagain;
	default:
		RET_UNKNOWN(ret);
	}

	return ret_eagain;
}


static void
prepare_and_set_post (cherokee_admin_client_t *admin, char *str, cuint_t str_len)
{
	cherokee_downloader_t *downloader = DOWNLOADER(admin->downloader);

	cherokee_downloader_set_url (downloader, admin->url_ref); 
	cherokee_buffer_add (&admin->request, str, str_len); 

	/* Build and set the post object
	 */
	cherokee_post_init (&admin->post);
	cherokee_post_set_len (&admin->post, str_len);
	cherokee_post_append (&admin->post, str, str_len);
	cherokee_downloader_post_set (downloader, &admin->post); 
}

#define SET_POST(admin,str) \
	prepare_and_set_post(admin, str"\n", sizeof(str))


#define CHECK_AND_SKIP_LITERAL(string, substr) \
	if ((string == NULL) || (strlen(string) == 0)) \
		return ret_error; \
	if (strncmp (string, substr, sizeof(substr)-1)) { \
		PRINT_ERROR ("ERROR: Uknown response len(" FMT_SIZE "): '%s'\n", \
			     (CST_SIZE) strlen(string), string); \
		return ret_error; \
	} \
	string += sizeof(substr)-1;


static ret_t
check_and_skip_literal (cherokee_buffer_t *buf, const char *literal) 
{
	cint_t  re;
	cuint_t len;
	
	len = strlen(literal);
	cherokee_buffer_trim (buf);

	re = strncmp (buf->buf, literal, len);
	if (re != 0) {
#if 0
		PRINT_ERROR ("ERROR: Couldn't find len(%d):'%s' in len(%d):'%s'\n", 
			     strlen(literal), literal, buf->len, buf->buf);
#endif
		return ret_error;
	}

	cherokee_buffer_move_to_begin (buf, len);
	return ret_ok;
}


static ret_t
common_processing (cherokee_admin_client_t *admin, 
		   void (*conf_request_func) (cherokee_admin_client_t *admin, void *argument),
		   void *argument) 
{
	ret_t                  ret;
	cherokee_downloader_t *downloader = DOWNLOADER(admin->downloader);

	TRACE(ENTRIES, "Downloader phase: %d\n", downloader->phase);

	/* Initial state: needs to get the Post info
	 */
	if ((downloader->phase == downloader_phase_init) &&
	    (downloader->post == NULL))
	{
		conf_request_func (admin, argument);
		return ret_eagain;
	}

	/* Finished 
	 */
	if (downloader->phase == downloader_phase_finished)
		return ret_ok;

	/* It's iterating
	 */
	ret = internal_step (admin);
	return ret;
}


/* Port
 */
static void
ask_get_port (cherokee_admin_client_t *admin, void *arg)
{
	UNUSED(arg);
	SET_POST (admin, "get server.port");
}

static ret_t
parse_reply_get_port (cherokee_buffer_t *reply, cuint_t *port)
{
	ret_t ret;

	ret = check_and_skip_literal (reply, "server.port is ");
	if (ret != ret_ok) return ret;

	*port = strtol (reply->buf, NULL, 10);
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_port (cherokee_admin_client_t *admin, cuint_t *port)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_port, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_port (&DOWNLOADER(admin->downloader)->body, port);
}


/* Port TLS
 */
static void
ask_get_port_tls (cherokee_admin_client_t *admin, void *arg)
{
	UNUSED(arg);
	SET_POST (admin, "get server.port_tls");
}

static ret_t
parse_reply_get_port_tls (cherokee_buffer_t *reply, cuint_t *port)
{
	ret_t ret;

	ret = check_and_skip_literal (reply, "server.port_tls is ");
	if (ret != ret_ok) return ret;

	*port = strtol (reply->buf, NULL, 10);
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_port_tls (cherokee_admin_client_t *admin, cuint_t *port)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_port_tls, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_port_tls (&DOWNLOADER(admin->downloader)->body, port);
}


/* RX 
 */
static void
ask_get_rx (cherokee_admin_client_t *admin, void *arg)
{
	UNUSED(arg);
	SET_POST (admin, "get server.rx");
}

static ret_t
parse_reply_get_rx (cherokee_buffer_t *reply, cherokee_buffer_t *rx)
{
	ret_t ret;

	ret = check_and_skip_literal (reply, "server.rx is ");
	if (ret != ret_ok) return ret;

	cherokee_buffer_add_buffer (rx, reply);
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_rx (cherokee_admin_client_t *admin, cherokee_buffer_t *rx)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_rx, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_rx (&DOWNLOADER(admin->downloader)->body, rx);	
}


/* TX 
 */
static void
ask_get_tx (cherokee_admin_client_t *admin, void *arg)
{
	UNUSED(arg);
	SET_POST (admin, "get server.tx");
}

static ret_t
parse_reply_get_tx (cherokee_buffer_t *reply, cherokee_buffer_t *tx)
{
	ret_t ret;

	ret = check_and_skip_literal (reply, "server.tx is ");
	if (ret != ret_ok) return ret;

	cherokee_buffer_add_buffer (tx, reply);
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_tx (cherokee_admin_client_t *admin, cherokee_buffer_t *tx)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_tx, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_tx (&DOWNLOADER(admin->downloader)->body, tx);	
}


/* Connections
 */
static void
ask_get_connections (cherokee_admin_client_t *admin, void *arg)
{
	UNUSED(arg);
	SET_POST (admin, "get server.connections");
}

static ret_t
parse_reply_get_connections (char *reply, cherokee_list_t *conns_list)
{
	char              *begin;
	char              *end;
	cherokee_buffer_t  info_str = CHEROKEE_BUF_INIT;

	char *p = reply;
	
	CHECK_AND_SKIP_LITERAL (reply, "server.connections are ");

	for (;;) {
		char                       *string;
		char                       *token;
		cherokee_connection_info_t *conn_info;

		begin = strchr (p, '[');
		end   = strchr (p, ']');
	
		if ((begin == NULL) || (end == NULL) || (end < begin))
			return ret_ok;

		begin++;
		p = end + 1;

		cherokee_buffer_add (&info_str, begin, end - begin);
		cherokee_connection_info_new (&conn_info);

		string = info_str.buf;
		while ((token = (char *) strsep(&string, ",")) != NULL)
		{
			char *equal;

			if (token == NULL) continue;

			equal = strchr (token, '=');
			if (equal == NULL) continue;
			equal++;

			if (!strncmp (token, "request=", 8))
				cherokee_buffer_add (&conn_info->request, equal, strlen(equal));
			else if (!strncmp (token, "phase=", 6))
				cherokee_buffer_add (&conn_info->phase, equal, strlen(equal));
			else if (!strncmp (token, "rx=", 3))
				cherokee_buffer_add (&conn_info->rx, equal, strlen(equal));
			else if (!strncmp (token, "tx=", 3))
				cherokee_buffer_add (&conn_info->tx, equal, strlen(equal));			
			else if (!strncmp (token, "total_size=", 11))
				cherokee_buffer_add (&conn_info->total_size, equal, strlen(equal));			
			else if (!strncmp (token, "ip=", 3))
				cherokee_buffer_add (&conn_info->ip, equal, strlen(equal));			
			else if (!strncmp (token, "id=", 3)) 
				cherokee_buffer_add (&conn_info->id, equal, strlen(equal));
			else if (!strncmp (token, "percent=", 8)) 
				cherokee_buffer_add (&conn_info->percent, equal, strlen(equal));
			else if (!strncmp (token, "handler=", 8)) 
				cherokee_buffer_add (&conn_info->handler, equal, strlen(equal));
			else if (!strncmp (token, "icon=", 5)) 
				cherokee_buffer_add (&conn_info->icon, equal, strlen(equal));
			else
				SHOULDNT_HAPPEN;
		}

		cherokee_list_add (LIST(conn_info), conns_list);
		cherokee_buffer_clean (&info_str);
	}

	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_connections (cherokee_admin_client_t *admin, cherokee_list_t *conns_list)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_connections, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_connections (DOWNLOADER(admin->downloader)->body.buf, conns_list);		
}


/* Delete connections
 */
static void
ask_del_connection (cherokee_admin_client_t *admin, void *arg)
{
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_va (&tmp, "del server.connection %s\n", (char *)arg);
	prepare_and_set_post (admin, tmp.buf, tmp.len);
	cherokee_buffer_mrproper (&tmp);
}

static ret_t
parse_reply_del_connection (char *reply, char *id)
{
	ret_t             ret;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_va (&tmp, "server.connection %s has been deleted", id);
	ret = (!strcmp_begin (reply, tmp.buf))? ret_ok : ret_error;
	cherokee_buffer_mrproper (&tmp);

	return ret;
}

ret_t 
cherokee_admin_client_del_connection  (cherokee_admin_client_t *admin, char *id)
{
	ret_t ret;

	ret = common_processing (admin, ask_del_connection, id);
	if (ret != ret_ok) return ret;

	return parse_reply_del_connection (DOWNLOADER(admin->downloader)->body.buf, id);	
}


/* Thread number
 */
static void
ask_thread_number (cherokee_admin_client_t *admin, void *arg)
{
	UNUSED(arg);
	SET_POST (admin, "get server.thread_num");
}

static ret_t
parse_reply_thread_number (cherokee_buffer_t *reply, cherokee_buffer_t *num)
{
	ret_t ret;

	ret = check_and_skip_literal (reply, "server.thread_num is ");
	if (ret != ret_ok) return ret;

	cherokee_buffer_add_buffer (num, reply);
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_thread_num  (cherokee_admin_client_t *admin, cherokee_buffer_t *num)
{
	ret_t ret;

	ret = common_processing (admin, ask_thread_number, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_thread_number (&DOWNLOADER(admin->downloader)->body, num);
}


/* Backup mode
 */
static void
set_backup_mode (cherokee_admin_client_t *admin, void *arg)
{
	cherokee_boolean_t active = POINTER_TO_INT(arg);

	if (active) {
		SET_POST (admin, "set server.backup_mode on\n");
	} else {
		SET_POST (admin, "set server.backup_mode off\n");
	}
}

static ret_t
parse_reply_set_backup_mode (char *reply, cherokee_boolean_t active)
{

	if (active) {
		CHECK_AND_SKIP_LITERAL (reply, "server.backup_mode is on");
	} else {
		CHECK_AND_SKIP_LITERAL (reply, "server.backup_mode is off");
	}

	return ret_ok;
}

ret_t 
cherokee_admin_client_set_backup_mode (cherokee_admin_client_t *admin, cherokee_boolean_t active)
{
	ret_t ret;

	ret = common_processing (admin, set_backup_mode, INT_TO_POINTER(active));
	if (ret != ret_ok) return ret;

	return parse_reply_set_backup_mode (DOWNLOADER(admin->downloader)->body.buf, active);	
}


/* Tracing
 */
static void
ask_trace (cherokee_admin_client_t *admin, void *arg)
{
	UNUSED(arg);
	SET_POST (admin, "get server.trace");
}

static ret_t
parse_reply_trace (cherokee_buffer_t *reply, cherokee_buffer_t *trace)
{
	ret_t ret;

	ret = check_and_skip_literal (reply, "server.trace is ");
	if (ret != ret_ok) return ret;

	cherokee_buffer_add_buffer (trace, reply);
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_trace (cherokee_admin_client_t *admin, cherokee_buffer_t *trace)
{
	ret_t ret;

	ret = common_processing (admin, ask_trace, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_trace (&DOWNLOADER(admin->downloader)->body, trace);
}


static ret_t
check_reply_trace (cherokee_buffer_t *reply, cherokee_buffer_t *trace)
{
	ret_t ret;

	UNUSED(trace);

	ret = check_and_skip_literal (reply, "ok");
	if (ret != ret_ok) return ret;

	return ret_ok;
}

static void
set_trace_mode (cherokee_admin_client_t *admin, void *arg)
{
	cherokee_buffer_t *trace = arg;
	cherokee_buffer_t  tmp   = CHEROKEE_BUF_INIT;
	
	cherokee_buffer_add_str (&tmp, "set server.trace ");
	cherokee_buffer_add_buffer (&tmp, trace);
	cherokee_buffer_add_str (&tmp, "\n");

	prepare_and_set_post (admin, tmp.buf, tmp.len);
	cherokee_buffer_mrproper (&tmp);
}

ret_t 
cherokee_admin_client_set_trace (cherokee_admin_client_t *admin, cherokee_buffer_t *trace)
{
	ret_t ret;

	ret = common_processing (admin, set_trace_mode, trace);
	if (ret != ret_ok) return ret;

	return check_reply_trace (&DOWNLOADER(admin->downloader)->body, trace);
}
