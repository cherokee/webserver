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

#ifndef CHEROKEE_HANDLER_PROXY_HOST_H
#define CHEROKEE_HANDLER_PROXY_HOST_H

#include "common-internal.h"
#include "avl.h"
#include "list.h"
#include "source.h"
#include "socket.h"

typedef enum {
	pconn_enc_none,
	pconn_enc_known_size,
	pconn_enc_chunked
} cherokee_handler_proxy_enc_t;

typedef struct {
	/* Connection poll */
	cherokee_avl_t            hosts;
	CHEROKEE_MUTEX_T         (hosts_mutex);
	cherokee_buffer_t         tmp;
} cherokee_handler_proxy_hosts_t;

typedef struct {
	CHEROKEE_MUTEX_T (mutex);
	cherokee_list_t   active;
	cherokee_list_t   reuse;
	cuint_t           reuse_len;
	cuint_t           reuse_max;
} cherokee_handler_proxy_poll_t;

typedef struct {
	cherokee_list_t                listed;
	cherokee_socket_t              socket;
	cherokee_handler_proxy_poll_t *poll_ref;

	/* Name resolution */
	const struct addrinfo         *addr_info_ref;
	cuint_t                        addr_total;
	cuint_t                        addr_current;

	/* In */
	cherokee_handler_proxy_enc_t   enc;
	cherokee_buffer_t              header_in_raw;
	cherokee_boolean_t             keepalive_in;
	size_t                         size_in;

	/* Out */
	size_t                         sent_out;
	struct {
		cherokee_buffer_t      buf_temp;
		cherokee_boolean_t     do_buf_sent;
		off_t                  sent;
	} post;
} cherokee_handler_proxy_conn_t;

#define PROXY_HOSTS(h) ((cherokee_handler_proxy_hosts_t *)(h))
#define PROXY_POLL(p)  ((cherokee_handler_proxy_poll_t *)(p))
#define PROXY_CONN(c)  ((cherokee_handler_proxy_conn_t *)(c))


/* Hosts
 */
ret_t cherokee_handler_proxy_hosts_init     (cherokee_handler_proxy_hosts_t *hosts);
ret_t cherokee_handler_proxy_hosts_mrproper (cherokee_handler_proxy_hosts_t *hosts);

ret_t cherokee_handler_proxy_hosts_get      (cherokee_handler_proxy_hosts_t  *hosts,
                                             cherokee_source_t               *src,
                                             cherokee_handler_proxy_poll_t  **poll,
                                             cuint_t                          reuse_max);

/* Polls
 */
ret_t cherokee_handler_proxy_poll_new       (cherokee_handler_proxy_poll_t  **poll,
                                             cuint_t                          reuse_max);
ret_t cherokee_handler_proxy_poll_free      (cherokee_handler_proxy_poll_t   *poll);
ret_t cherokee_handler_proxy_poll_get       (cherokee_handler_proxy_poll_t   *poll,
                                             cherokee_handler_proxy_conn_t  **pconn,
                                             cherokee_source_t               *src);


/* Conns
 */
ret_t cherokee_handler_proxy_conn_new          (cherokee_handler_proxy_conn_t **pconn);
ret_t cherokee_handler_proxy_conn_free         (cherokee_handler_proxy_conn_t  *pconn);
ret_t cherokee_handler_proxy_conn_release      (cherokee_handler_proxy_conn_t  *pconn);
ret_t cherokee_handler_proxy_conn_send         (cherokee_handler_proxy_conn_t  *pconn,
                                                cherokee_buffer_t              *buf);
ret_t cherokee_handler_proxy_conn_recv_headers (cherokee_handler_proxy_conn_t  *pconn,
                                                cherokee_buffer_t              *body,
                                                cherokee_boolean_t              flexible);
ret_t cherokee_handler_proxy_conn_get_addrinfo (cherokee_handler_proxy_conn_t  *pconn,
                                                cherokee_source_t              *src);
ret_t cherokee_handler_proxy_conn_init_socket  (cherokee_handler_proxy_conn_t  *pconn,
                                                cherokee_source_t              *src);

#endif /* CHEROKEE_HANDLER_PROXY_HOSTS_H */


