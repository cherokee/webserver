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
#include "cryptor.h"
#include "logger_writer.h"
#include "collector.h"
#include "post_track.h"

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

	/* Tables: nonces, etc
	 */
	cherokee_regex_table_t    *regexs;
	cherokee_nonce_table_t    *nonces;

	/* Programmed tasks
	 */
	int                        nonces_cleanup_lapse;
	time_t                     nonces_cleanup_next;

	int                        flcache_lapse;
	time_t                     flcache_next;

	/* Logging
	 */
	cherokee_logger_writer_t  *error_writer;

	cherokee_list_t            logger_writers;
	cherokee_avl_t             logger_writers_index;

	int                        log_flush_lapse;
	time_t                     log_flush_next;

	/* Extensions
	 */
	cherokee_cryptor_t        *cryptor;
	cherokee_post_track_t     *post_track;
	cherokee_collector_t      *collector;

	/* System related
	 */
	int                        fdlimit_custom;
	int                        fdlimit_available;
	cherokee_poll_type_t       fdpoll_method;

	/* Connection related
	 */
	cuint_t                    conns_max;
	cint_t                     conns_reuse_max;

	cherokee_boolean_t         keepalive;
	cuint_t                    keepalive_max;
	cherokee_boolean_t         chunked_encoding;

	/* Networking config
	 */
	cherokee_boolean_t         ipv6;
	int                        fdwatch_msecs;
	int                        listen_queue;
	cherokee_boolean_t         tls_enabled;

	cherokee_list_t            listeners;
	CHEROKEE_MUTEX_T          (listeners_mutex);

	/* Server name
	 */
	cherokee_server_token_t    server_token;

	/* User/group and chroot
	 */
	uid_t                      user;
	uid_t                      user_orig;
	gid_t                      group;
	gid_t                      group_orig;

	cherokee_buffer_t          chroot;
	cherokee_boolean_t         chrooted;

	/* I/O cache
	 */
	cherokee_iocache_t        *iocache;
	cherokee_boolean_t         iocache_enabled;

	/* Other objects
	 */
	cherokee_mime_t           *mime;
	cherokee_icons_t          *icons;
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

	/* Paths
	 */
	cherokee_buffer_t          pidfile;
	cherokee_buffer_t          themes_dir;
};


ret_t cherokee_server_close_connection (cherokee_server_t *srv, cherokee_thread_t *mythread, char *id_str);
ret_t cherokee_server_get_vserver      (cherokee_server_t *srv, cherokee_buffer_t *name, cherokee_connection_t *conn, cherokee_virtual_server_t **vsrv);
ret_t cherokee_server_get_next_bind    (cherokee_server_t *srv, cherokee_bind_t *bind, cherokee_bind_t **next);
ret_t cherokee_server_get_log_writer   (cherokee_server_t *srv, cherokee_config_node_t *config, cherokee_logger_writer_t **writer);

#endif /* CHEROKEE_SERVER_PROTECTED_H */
