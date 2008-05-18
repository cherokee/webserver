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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_ADMIN_CLIENT_H
#define CHEROKEE_ADMIN_CLIENT_H

#include <cherokee/common.h>
#include <cherokee/http.h>
#include <cherokee/fdpoll.h>
#include <cherokee/buffer.h>
#include <cherokee/connection_info.h>

CHEROKEE_BEGIN_DECLS


typedef struct cherokee_admin_client cherokee_admin_client_t;
#define ADMIN_CLIENT(x) ((cherokee_admin_client_t *)(x))

#define RUN_CLIENT_LOOP(func_string) {        \
	cherokee_boolean_t exit;              \
	                                      \
	for (exit = false; !exit;) {          \
		ret = func_string;            \
		switch (ret) {                \
		case ret_error:               \
			ret = ret_error;      \
			exit = true;          \
			break;                \
		case ret_ok:                  \
			ret = ret_ok;         \
			exit = true;          \
			break;                \
		case ret_eagain:              \
			break;                \
		case ret_eof:                 \
			exit = true;          \
			break;                \
		default:                      \
			RET_UNKNOWN(ret);     \
		}                             \
	}                                     \
}

#define RUN_CLIENT1(client,func,arg)            \
	cherokee_admin_client_reuse(client);    \
        RUN_CLIENT_LOOP(func(client,arg))

#define RUN_CLIENT2(client,func,arg1,arg2)      \
	cherokee_admin_client_reuse(client);    \
	RUN_CLIENT_LOOP(func(client,arg1,arg2))

#define RUN_CLIENT3(client,func,arg1,arg2,arg3) \
	cherokee_admin_client_reuse(client);    \
	RUN_CLIENT_LOOP(func(client,arg1,arg2,arg3))


ret_t cherokee_admin_client_new             (cherokee_admin_client_t **admin);
ret_t cherokee_admin_client_free            (cherokee_admin_client_t  *admin);

ret_t cherokee_admin_client_prepare         (cherokee_admin_client_t *admin, cherokee_fdpoll_t *poll, cherokee_buffer_t *url, cherokee_buffer_t *user, cherokee_buffer_t *pass);
ret_t cherokee_admin_client_connect         (cherokee_admin_client_t *admin);
ret_t cherokee_admin_client_reuse           (cherokee_admin_client_t *admin);
ret_t cherokee_admin_client_get_reply_code  (cherokee_admin_client_t *admin, cherokee_http_t *code);

/* Retrieve information methods
 */
ret_t cherokee_admin_client_ask_port        (cherokee_admin_client_t *admin, cuint_t *port);
ret_t cherokee_admin_client_ask_port_tls    (cherokee_admin_client_t *admin, cuint_t *port);

ret_t cherokee_admin_client_ask_rx          (cherokee_admin_client_t *admin, cherokee_buffer_t *rx);
ret_t cherokee_admin_client_ask_tx          (cherokee_admin_client_t *admin, cherokee_buffer_t *tx);

ret_t cherokee_admin_client_ask_connections (cherokee_admin_client_t *admin, cherokee_list_t *conns);
ret_t cherokee_admin_client_del_connection  (cherokee_admin_client_t *admin, char *id);

ret_t cherokee_admin_client_ask_thread_num  (cherokee_admin_client_t *admin, cherokee_buffer_t *num);
ret_t cherokee_admin_client_set_backup_mode (cherokee_admin_client_t *admin, cherokee_boolean_t active);

ret_t cherokee_admin_client_ask_trace       (cherokee_admin_client_t *admin, cherokee_buffer_t *trace);
ret_t cherokee_admin_client_set_trace       (cherokee_admin_client_t *admin, cherokee_buffer_t *trace);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_ADMIN_CLIENT_H */
