/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
#include "downloader.h"
#include "downloader-protected.h"


typedef enum {
	phase_init,
	phase_stepping,
	phase_finished
} phase_t;


struct cherokee_admin_client {
	cherokee_buffer_t        *url_ref;
	cherokee_buffer_t         request;
	cherokee_buffer_t         reply;

	phase_t                   phase;
	cherokee_fdpoll_t        *poll_ref;
	cherokee_downloader_t     downloader;
};

#define strcmp_begin(line,sub)       strncmp(line, sub, strlen(sub))
#define strcmp_begin_const(line,sub) strncmp(line, sub, sizeof(sub))


ret_t 
cherokee_admin_client_new (cherokee_admin_client_t **admin)
{
	CHEROKEE_NEW_STRUCT(n,admin_client);

	/* Init
	 */
	n->phase        = phase_init;

	n->poll_ref     = NULL;
	n->url_ref      = NULL;

	cherokee_downloader_init (&n->downloader);
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

	cherokee_downloader_mrproper (&admin->downloader);

	free (admin);
	return ret_ok;
}


ret_t 
cherokee_admin_client_set_fdpoll (cherokee_admin_client_t *admin, cherokee_fdpoll_t *poll)
{
	if (admin->poll_ref != NULL) {
		PRINT_ERROR_S ("WARNING: Overwritting poll object\n");
	}

	return ret_ok;
}


static ret_t
on_downloader_finish (void *_downloader, void *_param)
{
	cherokee_admin_client_t *admin = _param;

	admin->phase = phase_finished;
	return ret_ok;
}


ret_t 
cherokee_admin_client_prepare (cherokee_admin_client_t *admin, cherokee_fdpoll_t *poll, cherokee_buffer_t *url)
{
	ret_t                  ret;
	cherokee_downloader_t *downloader = &admin->downloader;

	admin->phase    = phase_init;

	admin->poll_ref = poll;
	admin->url_ref  = url;

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
	ret = cherokee_downloader_set_fdpoll (downloader, admin->poll_ref);
	if (unlikely (ret != ret_ok)) return ret;
	
	ret = cherokee_downloader_set_url (&admin->downloader, admin->url_ref); 
	if (unlikely (ret != ret_ok)) return ret;

	ret = cherokee_downloader_set_keepalive (downloader, true);
	if (unlikely (ret != ret_ok)) return ret;

	ret = cherokee_downloader_connect_event (downloader, downloader_event_finish, on_downloader_finish, admin);
	if (unlikely (ret != ret_ok)) return ret;

	return ret_ok;
}


ret_t 
cherokee_admin_client_connect (cherokee_admin_client_t *admin)
{
	return cherokee_downloader_connect (&admin->downloader);
}

ret_t 
cherokee_admin_client_get_reply_code (cherokee_admin_client_t *admin, cherokee_http_t *code)
{
	return cherokee_downloader_get_reply_code (&admin->downloader, code);
}

ret_t 
cherokee_admin_client_reuse (cherokee_admin_client_t *admin)
{
	cherokee_downloader_reuse (&admin->downloader);

	cherokee_buffer_clean (&admin->request);
	cherokee_buffer_clean (&admin->reply);

	admin->phase = phase_init;
	return ret_ok;
}


ret_t
cherokee_admin_client_internal_step (cherokee_admin_client_t *admin)
{
	ret_t                  ret;
	cherokee_downloader_t *downloader = &admin->downloader;

	/* Has it finished?
	 */
	if (admin->phase == phase_finished) 
		return ret_ok;

	/* Sanity check
	 */
	if (admin->phase != phase_stepping) 
		return ret_error;

	/* It's stepping
	 */
	ret = cherokee_downloader_step (downloader);
	switch (ret) {
	case ret_eof:
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
	cherokee_downloader_t *downloader = &admin->downloader;

	cherokee_buffer_add (&admin->request, str, str_len); 

	cherokee_downloader_reuse (downloader); 
	cherokee_downloader_set_url (downloader, admin->url_ref); 
	cherokee_downloader_post_set (downloader, &admin->request); 

	admin->phase = phase_stepping;
}

#define SET_POST(admin,str) \
	prepare_and_set_post(admin, str"\n", sizeof(str))


#define CHECK_AND_SKIP_LITERAL(string, substr) \
	if ((string == NULL) || (strlen(string) == 0)) \
		return ret_error; \
	if (strncmp (string, substr, sizeof(substr)-1)) { \
		if (0) PRINT_ERROR ("ERROR: Uknown response len(%d): '%s'\n", strlen(string), string); \
		return ret_error; \
	} \
	string += sizeof(substr)-1;


static ret_t
common_processing (cherokee_admin_client_t *admin, 
		   void (*conf_request_func) (cherokee_admin_client_t *admin, void *argument),
		   void *argument) 
{
	ret_t ret;

	switch (admin->phase) {
	case phase_init:
		conf_request_func (admin, argument);
		return ret_eagain;
	case phase_stepping:
		ret = cherokee_admin_client_internal_step (admin);
		return ret;
	case phase_finished:
		return ret_ok;
	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}


/* Port
 */
static void
ask_get_port (cherokee_admin_client_t *admin, void *arg)
{
	SET_POST (admin, "get server.port");
}

static ret_t
parse_reply_get_port (char *reply, cuint_t *port)
{
	CHECK_AND_SKIP_LITERAL (reply, "server.port is ");
	*port = strtol (reply, NULL, 10);
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_port (cherokee_admin_client_t *admin, cuint_t *port)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_port, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_port (admin->downloader.body.buf, port);
}


/* Port TLS
 */
static void
ask_get_port_tls (cherokee_admin_client_t *admin, void *arg)
{
	SET_POST (admin, "get server.port_tls");
}

static ret_t
parse_reply_get_port_tls (char *reply, cuint_t *port)
{
	CHECK_AND_SKIP_LITERAL (reply, "server.port_tls is ");

	*port = strtol (reply, NULL, 10);
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_port_tls (cherokee_admin_client_t *admin, cuint_t *port)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_port_tls, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_port_tls (admin->downloader.body.buf, port);
}


/* RX 
 */
static void
ask_get_rx (cherokee_admin_client_t *admin, void *arg)
{
	SET_POST (admin, "get server.rx");
}

static ret_t
parse_reply_get_rx (char *reply, cherokee_buffer_t *rx)
{
	CHECK_AND_SKIP_LITERAL (reply, "server.rx is ");

	cherokee_buffer_add (rx, reply, strlen(reply));
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_rx (cherokee_admin_client_t *admin, cherokee_buffer_t *rx)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_rx, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_rx (admin->downloader.body.buf, rx);	
}


/* TX 
 */
static void
ask_get_tx (cherokee_admin_client_t *admin, void *args)
{
	SET_POST (admin, "get server.tx");
}

static ret_t
parse_reply_get_tx (char *reply, cherokee_buffer_t *tx)
{
	CHECK_AND_SKIP_LITERAL (reply, "server.tx is ");

	cherokee_buffer_add (tx, reply, strlen(reply));
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_tx (cherokee_admin_client_t *admin, cherokee_buffer_t *tx)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_tx, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_tx (admin->downloader.body.buf, tx);	
}


/* Connections
 */
static void
ask_get_connections (cherokee_admin_client_t *admin, void *arg)
{
	SET_POST (admin, "get server.connections");
}

static ret_t
parse_reply_get_connections (char *reply, list_t *conns_list)
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

		list_add ((list_t *)conn_info, conns_list);
		cherokee_buffer_clean (&info_str);
	}

	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_connections (cherokee_admin_client_t *admin, list_t *conns_list)
{
	ret_t ret;

	ret = common_processing (admin, ask_get_connections, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_get_connections (admin->downloader.body.buf, conns_list);		
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

	return parse_reply_del_connection (admin->downloader.body.buf, id);	
}


/* Thread number
 */
static void
ask_thread_number (cherokee_admin_client_t *admin, void *arg)
{
	SET_POST (admin, "get server.thread_num");
}

static ret_t
parse_reply_thread_number (char *reply, cherokee_buffer_t *num)
{
	CHECK_AND_SKIP_LITERAL (reply, "server.thread_num is ");

	cherokee_buffer_add (num, reply, strlen(reply));
	return ret_ok;
}

ret_t 
cherokee_admin_client_ask_thread_num  (cherokee_admin_client_t *admin, cherokee_buffer_t *num)
{
	ret_t ret;

	ret = common_processing (admin, ask_thread_number, NULL);
	if (ret != ret_ok) return ret;

	return parse_reply_thread_number (admin->downloader.body.buf, num);
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

	return parse_reply_set_backup_mode (admin->downloader.body.buf, active);	
}
