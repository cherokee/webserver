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

#ifndef CHEROKEE_HANDLER_PROXY_HOST_H
#define CHEROKEE_HANDLER_PROXY_HOST_H

#include "common-internal.h"
#include "avl.h"
#include "list.h"
#include "source.h"
#include "socket.h"

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
} cherokee_handler_proxy_poll_t;

typedef struct {
	cherokee_list_t                listed;
	cherokee_socket_t              socket;
	cherokee_handler_proxy_poll_t *poll_ref;
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
					     cherokee_handler_proxy_poll_t  **poll);

/* Polls
 */
ret_t cherokee_handler_proxy_poll_new       (cherokee_handler_proxy_poll_t  **poll);
ret_t cherokee_handler_proxy_poll_free      (cherokee_handler_proxy_poll_t   *poll);
ret_t cherokee_handler_proxy_poll_get       (cherokee_handler_proxy_poll_t   *poll,
					     cherokee_handler_proxy_conn_t  **pconn, 
					     cherokee_source_t               *src);

/* Conns
 */
ret_t cherokee_handler_proxy_conn_new       (cherokee_handler_proxy_conn_t **pconn);
ret_t cherokee_handler_proxy_conn_free      (cherokee_handler_proxy_conn_t  *pconn);
ret_t cherokee_handler_proxy_conn_release   (cherokee_handler_proxy_conn_t  *pconn);

ret_t cherokee_handler_proxy_conn_send      (cherokee_handler_proxy_conn_t  *pconn,
					     cherokee_buffer_t              *buf);
ret_t cherokee_handler_proxy_conn_recv      (cherokee_handler_proxy_conn_t  *pconn,
					     cherokee_buffer_t              *buf);


#endif /* CHEROKEE_HANDLER_PROXY_HOSTS_H */


