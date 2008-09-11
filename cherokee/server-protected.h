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

#ifndef CHEROKEE_SERVER_PROTECTED_H
#define CHEROKEE_SERVER_PROTECTED_H

#include "common-internal.h"

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else 
# include <time.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include "server.h"
#include "list.h"
#include "fdpoll.h"
#include "virtual_server.h"
#include "thread.h"
#include "plugin_loader.h"
#include "icons.h"
#include "iocache.h"
#include "regex.h"
#include "nonce.h"
#include "mime.h"
#include "config_node.h"
#include "version.h"


struct cherokee_server {
	/* Exit related
	 */
	time_t                     start_time;
	cherokee_buffer_t          panic_action;

	/* Restarts
	 */
	cherokee_boolean_t         wanna_exit;
	cherokee_boolean_t         wanna_reinit;

	/* Virtual servers
	 */
	cherokee_list_t            vservers;
	
	/* Threads
	 */
	cherokee_thread_t         *main_thread;
	cint_t                     thread_num;
	cherokee_list_t            thread_list;
	cint_t                     thread_policy;

	/* Modules
	 */
	cherokee_plugin_loader_t   loader;
	cherokee_avl_t             encoders;

	/* Tables: iocache, nonces, etc
	 */
	cherokee_regex_table_t    *regexs;
	cherokee_nonce_table_t    *nonces;

	/* Logging
	 */
	int                        log_flush_elapse;
	time_t                     log_flush_next;

	/* Main socket
	 */
	cherokee_socket_t          socket;
	cherokee_socket_t          socket_tls;

	CHEROKEE_MUTEX_T          (accept_mutex);
#ifdef HAVE_TLS
	CHEROKEE_MUTEX_T          (accept_tls_mutex);
#endif

	/* System related
	 */
	int                        fdlimit_custom;
	int                        fdlimit_available;
	cherokee_poll_type_t       fdpoll_method;

	/* Connection related
	 */
	cuint_t                    conns_max;
	cint_t                     conns_reuse_max;
	cuint_t                    conns_num_bogo;

	cherokee_boolean_t         keepalive;
	cuint_t                    keepalive_max;
	cherokee_boolean_t         chunked_encoding;

	/* Networking config
	 */
	cherokee_boolean_t         ipv6;
	cherokee_buffer_t          listen_to;
	int                        fdwatch_msecs;
	int                        listen_queue;

	unsigned short             port;
	unsigned short             port_tls;
	cherokee_boolean_t         tls_enabled;

	/* Server name
	 */
	cherokee_server_token_t    server_token;

	cherokee_buffer_t          server_string;
	cherokee_buffer_t          server_string_ext;
	cherokee_buffer_t          server_string_w_port;
	cherokee_buffer_t          server_string_w_port_tls;

	/* User/group and chroot
	 */
	uid_t                      user;
	uid_t                      user_orig;
	gid_t                      group;
	gid_t                      group_orig;

	cherokee_buffer_t          chroot;
	cherokee_boolean_t         chrooted;

	/* Other objects
	 */
	cherokee_mime_t           *mime;
	cherokee_icons_t          *icons;
	cherokee_iocache_t        *iocache;
	cherokee_avl_t             sources;

	/* Time related
	 */
	int                        timeout;
	cherokee_buffer_t          timeout_header;

	struct {
		off_t min;
		off_t max;
	} sendfile;

	/* Another config files
	 */
	cherokee_config_node_t     config;

	/* PID
	 */
	cherokee_buffer_t          pidfile;
};


ret_t cherokee_server_del_connection (cherokee_server_t *srv, char *begin);
ret_t cherokee_server_get_vserver    (cherokee_server_t *srv, cherokee_buffer_t *name, cherokee_virtual_server_t **vsrv);

#endif /* CHEROKEE_SERVER_PROTECTED_H */
