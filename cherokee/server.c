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
#include "server-protected.h"
#include "server.h"
#include "services.h"
#include "bind.h"
#include "error_log.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#ifdef HAVE_GRP_H
# include <grp.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif

#include <errno.h>

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#include <signal.h>
#include <dirent.h>
#include <unistd.h>

#include "thread.h"
#include "socket.h"
#include "connection.h"
#include "mime.h"
#include "util.h"
#include "fdpoll.h"
#include "fdpoll-protected.h"
#include "regex.h"
#include "connection-protected.h"
#include "nonce.h"
#include "config_reader.h"
#include "init.h"
#include "bogotime.h"
#include "source_interpreter.h"
#include "post_track.h"

#define ENTRIES "core,server"
#define GRNAM_BUF_LEN 8192

ret_t
cherokee_server_new  (cherokee_server_t **srv)
{
	ret_t ret;
	CHEROKEE_CNEW_STRUCT(1, n, server);

	/* Thread list
	 */
	INIT_LIST_HEAD(&(n->thread_list));

	/* Sockets
	 */
	n->ipv6              = true;
	n->fdpoll_method     = cherokee_poll_UNSET;

	/* Exit related
	 */
	n->wanna_exit        = false;
	n->wanna_reinit      = false;

	/* Server config
	 */
	n->tls_enabled       = false;
	n->cryptor           = NULL;
	n->post_track        = NULL;

	n->timeout           = 5;
	n->fdwatch_msecs     = 1000;

	n->start_time        = time (NULL);

	n->keepalive         = true;
	n->keepalive_max     = MAX_KEEPALIVE;
	n->chunked_encoding  = true;

	n->thread_num        = -1;
	n->thread_policy     = -1;
	n->conns_max         =  0;
	n->conns_reuse_max   = -1;

	n->chrooted          = false;
	n->user_orig         = getuid();
	n->user              = n->user_orig;
	n->group_orig        = getgid();
	n->group             = n->group_orig;
	n->server_token      = cherokee_version_full;

	n->fdlimit_custom    = MAX (FD_NUM_CUSTOM_LIMIT, cherokee_fdlimit);
	n->fdlimit_available = -1;

	n->listen_queue      = 65534;
	n->sendfile.min      = SENDFILE_MIN_SIZE;
	n->sendfile.max      = SENDFILE_MAX_SIZE;

	n->error_writer      = NULL;
	n->regexs            = NULL;
	n->nonces            = NULL;

	n->mime              = NULL;
	n->icons             = NULL;
	n->regexs            = NULL;
	n->collector         = NULL;

	n->iocache           = NULL;
	n->iocache_enabled   = true;

	cherokee_buffer_init (&n->chroot);
	cherokee_buffer_init (&n->timeout_header);

	cherokee_buffer_init (&n->panic_action);
	cherokee_buffer_add_str (&n->panic_action, CHEROKEE_PANIC_PATH);

	/* Virtual servers list
	 */
	INIT_LIST_HEAD (&n->vservers);
	INIT_LIST_HEAD (&n->listeners);
	CHEROKEE_MUTEX_INIT (&n->listeners_mutex, CHEROKEE_MUTEX_FAST);

	/* Module loader
	 */
	cherokee_plugin_loader_init (&n->loader);

	/* Encoders
	 */
	cherokee_avl_init (&n->encoders);

	/* Logs
	 */
	cherokee_avl_init (&n->logger_writers_index);
	INIT_LIST_HEAD (&n->logger_writers);

	n->log_flush_next       = 0;
	n->log_flush_lapse      = LOGGER_FLUSH_LAPSE;

	/* Programmed tasks
	 */
	n->nonces_cleanup_next  = 0;
	n->nonces_cleanup_lapse = NONCE_CLEANUP_LAPSE;

	n->flcache_next         = 0;
	n->flcache_lapse        = FLCACHE_LAPSE;

	/* Paths
	 */
	cherokee_buffer_init (&n->pidfile);

	cherokee_buffer_init    (&n->themes_dir);
	cherokee_buffer_add_str (&n->themes_dir, CHEROKEE_THEMEDIR);

	/* Config
	 */
	cherokee_config_node_init (&n->config);

	/* Information sources: SCGI, FCGI
	 */
	cherokee_avl_init (&n->sources);

	/* Regexs
	 */
	cherokee_regex_table_new (&n->regexs);
	if (unlikely (n->regexs == NULL)) {
		goto error;
	}

	/* Active nonces
	 */
	ret = cherokee_nonce_table_new (&n->nonces);
	if (unlikely(ret < ret_ok)) {
		goto error;
	}

	/* Init the default error writer
	 */
	ret = cherokee_logger_writer_new_stderr (&n->error_writer);
	if (ret != ret_ok) {
		goto error;
	}

	ret = cherokee_logger_writer_open (n->error_writer);
	if (ret != ret_ok) {
		goto error;
	}

	cherokee_error_set_default (n->error_writer);

	/* Return the object
	 */
	*srv = n;
	return ret_ok;

error:
	cherokee_server_free (n);
	return ret_error;
}


static ret_t
destroy_thread (cherokee_thread_t *thread)
{
	cherokee_thread_wait_end (thread);
	cherokee_thread_free (thread);

	return ret_ok;
}


ret_t
cherokee_server_free (cherokee_server_t *srv)
{
	cherokee_list_t *i, *j;

	/* Flag the threads: server is exiting
	 */
	list_for_each (i, &srv->thread_list) {
		THREAD(i)->exit = true;
		CHEROKEE_MUTEX_UNLOCK (&srv->listeners_mutex);
	}

	/* Kill the child processes
	 */
	cherokee_avl_mrproper (AVL_GENERIC(&srv->sources), (cherokee_func_free_t)cherokee_source_free);

	/* Services mechanism
	 */
	cherokee_services_client_free();

	/* Threads
	 */
	list_for_each_safe (i, j, &srv->thread_list) {
		TRACE(ENTRIES, "Destroying thread %p\n", i);
		destroy_thread (THREAD(i));
	}

	TRACE(ENTRIES, "Destroying main_thread %p\n", srv->main_thread);
	cherokee_thread_free (srv->main_thread);

	/* File descriptors
	 */
	list_for_each_safe (i, j, &srv->listeners) {
		cherokee_list_del(i);
		cherokee_bind_free (BIND(i));
	}

	CHEROKEE_MUTEX_DESTROY (&srv->listeners_mutex);

	/* Attached objects
	 */
	cherokee_avl_mrproper (AVL_GENERIC(&srv->encoders), NULL);

	cherokee_mime_free (srv->mime);
	cherokee_icons_free (srv->icons);
	cherokee_regex_table_free (srv->regexs);

	if (srv->iocache) {
		cherokee_iocache_free (srv->iocache);
	}

	if (srv->cryptor) {
		cherokee_cryptor_free (srv->cryptor);
	}

	if (srv->post_track) {
		MODULE(srv->post_track)->free (srv->post_track);
	}

	if (srv->collector) {
		cherokee_collector_free (srv->collector);
	}

	cherokee_nonce_table_free (srv->nonces);

	/* Logs
	 */
	list_for_each_safe (i, j, &srv->logger_writers) {
		cherokee_logger_writer_free (LOGGER_WRITER(i));
	}

	cherokee_avl_mrproper (AVL_GENERIC(&srv->logger_writers_index), NULL);

	/* Virtual servers
	 */
	list_for_each_safe (i, j, &srv->vservers) {
		cherokee_virtual_server_free (VSERVER(i));
	}

	cherokee_buffer_mrproper (&srv->timeout_header);

	cherokee_buffer_mrproper (&srv->chroot);
	cherokee_buffer_mrproper (&srv->pidfile);
	cherokee_buffer_mrproper (&srv->themes_dir);
	cherokee_buffer_mrproper (&srv->panic_action);

	/* Error writer
	 */
	if (srv->error_writer != NULL) {
		cherokee_logger_writer_free (srv->error_writer);
		srv->error_writer = NULL;
		cherokee_error_set_default (srv->error_writer);
	}

	/* Module loader: It must be the last action to be performed
	 * because it will close all the opened modules.
	 */
	cherokee_plugin_loader_mrproper (&srv->loader);

	TRACE(ENTRIES, "The server %p has been freed\n", srv);
	free (srv);

	return ret_ok;
}


static ret_t
change_execution_user (cherokee_server_t *srv, struct passwd *ent)
{
	int error;

	/* Reset `groups' attributes.
	 */
	if (srv->user_orig == 0) {
		error = initgroups (ent->pw_name, srv->group);
		if (error == -1) {
			LOG_WARNING (CHEROKEE_ERROR_SERVER_INITGROUPS, ent->pw_name, srv->group);
		}
	}

	/* Change of group requested
	 */
	if (srv->group != srv->group_orig) {
		error = setgid (srv->group);
		if (error != 0) {
			LOG_WARNING (CHEROKEE_ERROR_SERVER_SETGID, srv->group, srv->group_orig);
		}
	}

	/* Change of user requested
	 */
	if (srv->user != srv->user_orig) {
		error = setuid (srv->user);
		if (error != 0) {
			LOG_WARNING (CHEROKEE_ERROR_SERVER_SETUID, srv->user, srv->user_orig);
		}
	}

	return ret_ok;
}


void
cherokee_server_set_min_latency (cherokee_server_t *srv, int msecs)
{
	srv->fdwatch_msecs = msecs;
}


static ret_t
print_banner (cherokee_server_t *srv)
{
#ifdef TRACE_ENABLED
	ret_t              ret;
	cherokee_buffer_t *buf;
#endif
	const char        *method;
	cherokee_list_t   *i;
	size_t             b   = 0;
	size_t             len = 0;
	cherokee_buffer_t  n   = CHEROKEE_BUF_INIT;

	/* First line
	 */
	cherokee_buffer_add_va (&n, "Cherokee Web Server %s (%s): ", PACKAGE_VERSION, __DATE__);

	/* Ports
	 */
	cherokee_list_get_len (&srv->listeners, &len);
	if (len <= 1) {
		cherokee_buffer_add_str (&n, "Listening on port ");
	} else {
		cherokee_buffer_add_str (&n, "Listening on ports ");
	}

	list_for_each (i, &srv->listeners) {
		cherokee_bind_t *bind = BIND(i);

		b += 1;
		if (! cherokee_buffer_is_empty(&bind->ip)) {
			if (strchr (bind->ip.buf, ':')) {
				cherokee_buffer_add_va (&n, "[%s]", bind->ip.buf);
			} else {
				cherokee_buffer_add_buffer (&n, &bind->ip);
			}
		} else {
			cherokee_buffer_add_str (&n, "ALL");
		}

		cherokee_buffer_add_char (&n, ':');
		cherokee_buffer_add_ulong10 (&n, bind->port);

		if (bind->socket.is_tls == TLS) {
			cherokee_buffer_add_str (&n, "(TLS)");
		}

		if (b < len)
			cherokee_buffer_add_str (&n, ", ");
	}

	/* Chroot
	 */
	if (srv->chrooted) {
		cherokee_buffer_add_str (&n, ", chrooted");
	}

	/* TLS / SSL
	 */
	if (srv->tls_enabled) {
		cherokee_module_get_name (MODULE(srv->cryptor), (const char **)&method);
		cherokee_buffer_add_va (&n, ", with TLS support via %s", method);
	} else {
		cherokee_buffer_add_str (&n, ", TLS disabled");
	}

	/* IPv6
	 */
#ifdef HAVE_IPV6
	if (srv->ipv6) {
		cherokee_buffer_add_str (&n, ", IPv6 enabled");
	} else
#endif
		cherokee_buffer_add_str (&n, ", IPv6 disabled");

	/* Polling method
	 */
	cherokee_fdpoll_get_method_str (srv->main_thread->fdpoll, &method);
	cherokee_buffer_add_va (&n, ", using %s", method);

	/* File descriptor limit
	 */
	cherokee_buffer_add_va (&n, ", %d fds system limit, max. %d connections",
	                        cherokee_fdlimit, srv->conns_max);

	/* I/O-cache
	 */
	if (srv->iocache) {
		cherokee_buffer_add_str (&n, ", caching I/O");
	}

	/* Threading stuff
	 */
	if (srv->thread_num <= 1) {
		cherokee_buffer_add_str (&n, ", single thread");
	} else {
		cherokee_buffer_add_va (&n, ", %d threads", srv->thread_num);
		cherokee_buffer_add_va (&n, ", %d connections per thread", srv->main_thread->conns_max);

		switch (srv->thread_policy) {
#ifdef HAVE_PTHREAD
		case SCHED_FIFO:
			cherokee_buffer_add_str (&n, ", FIFO scheduling policy");
			break;
		case SCHED_RR:
			cherokee_buffer_add_str (&n, ", RR scheduling policy");
			break;
#endif
		default:
			cherokee_buffer_add_str (&n, ", standard scheduling policy");
			break;
		}
	}

	/* Trace
	 */
#ifdef TRACE_ENABLED
	ret = cherokee_trace_get_trace (&buf);
	if ((ret == ret_ok) &&
	    (! cherokee_buffer_is_empty (buf)))
	{
		cherokee_buffer_add_va (&n, ", tracing '%s'", buf->buf);
	}
#endif

	/* Print it to stdout
	 */
	cherokee_buffer_split_lines (&n, TERMINAL_WIDTH, NULL);
	fprintf (stdout, "%s\n", n.buf);
	fflush (stdout);

	cherokee_buffer_mrproper (&n);

	return ret_ok;
}


static ret_t
initialize_server_threads (cherokee_server_t *srv)
{
	ret_t   ret;
	cint_t  i;
	size_t  listen_fds;
	cuint_t fds_per_thread;
	cuint_t conns_per_thread;
	cuint_t keepalive_per_thread;
	cuint_t conns_keepalive_max;

	/* Reset max. conns value
	 */
	srv->conns_max = 0;

	/* Set fd upper limit for threads.
	 */
#ifdef HAVE_PTHREAD
	if (srv->thread_num > (srv->fdlimit_available / FD_NUM_MIN_PER_THREAD))
		srv->thread_num = (srv->fdlimit_available / FD_NUM_MIN_PER_THREAD);

	if (srv->thread_num < 1)
		srv->thread_num = 1;
#else
	srv->thread_num = 1;
#endif

	/* Set fds and connections limits
	 */
	ret = cherokee_list_get_len (&srv->listeners, &listen_fds);
	if (ret != ret_ok) {
		return ret;
	}

	fds_per_thread  = (srv->fdlimit_available / srv->thread_num);
	fds_per_thread -= listen_fds;

	/* Get fdpoll limits.
	 */
	if (srv->fdpoll_method != cherokee_poll_UNSET) {
		cuint_t sys_fd_limit  = 0;
		cuint_t poll_fd_limit = 0;

		ret = cherokee_fdpoll_get_fdlimits (srv->fdpoll_method, &sys_fd_limit, &poll_fd_limit);
		if (ret != ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_GET_FDLIMIT,
			              (int) srv->fdpoll_method);
			return ret_error;
		}

		/* Test system fd limit (no autotune here)
		 */
		if ((sys_fd_limit > 0) &&
		    (cherokee_fdlimit > sys_fd_limit)) {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_FDS_SYS_LIMIT,
			              cherokee_fdlimit, sys_fd_limit);
			return ret_error;
		}

		/* If polling set limit has too many fds, then
		 * decrease that number.
		 */
		if ((poll_fd_limit > 0) &&
		    (fds_per_thread > poll_fd_limit))
		{
			LOG_WARNING (CHEROKEE_ERROR_SERVER_THREAD_POLL,
			             fds_per_thread, poll_fd_limit);
			fds_per_thread = poll_fd_limit - listen_fds;
		}
	}

	/* Max conn number: Supposes two fds per connection
	 */
	srv->conns_max = ((srv->fdlimit_available - listen_fds) / 2);
	if (srv->conns_max > 2)
		srv->conns_max -= 1;

	/* Max keep-alive connections:
	 * Limit over which a thread doesn't allow keepalive
	 */
	conns_keepalive_max = srv->conns_max - (srv->conns_max / 16);
	if (conns_keepalive_max + 6 > srv->conns_max)
		conns_keepalive_max = srv->conns_max - 6;

	/* Per thread limits
	 */
	conns_per_thread     = (srv->conns_max / srv->thread_num);
	keepalive_per_thread = (conns_keepalive_max / srv->thread_num);

	/* Create the main thread (only structures, not a real thread)
	 */
	ret = cherokee_thread_new (&srv->main_thread, srv, thread_sync,
	                           srv->fdpoll_method,
	                           cherokee_fdlimit,
	                           fds_per_thread,
	                           conns_per_thread,
	                           keepalive_per_thread);
	if (unlikely(ret < ret_ok)) {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_NEW_THREAD, ret);
		return ret;
	}

	/* If Cherokee runs in single thread mode, it has to add the
	 * server sockets to the fdpoll. They will remain in there.
	 */
	if (srv->thread_num == 1) {
		cherokee_list_t *j;

		list_for_each (j, &srv->listeners) {
			ret = cherokee_fdpoll_add (srv->main_thread->fdpoll,
			                           S_SOCKET_FD(BIND(j)->socket),
			                           FDPOLL_MODE_READ);
			if (ret < ret_ok)
				return ret;
		}
	}

	/* If Cherokee has been compiled in multi-thread mode,
	 * then it may need to launch other threads.
	 */
#ifdef HAVE_PTHREAD
	for (i = 0; i < srv->thread_num - 1; i++) {
		cherokee_thread_t *thread;

		/* Create a real thread.
		 */
		ret = cherokee_thread_new (&thread, srv, thread_async,
		                           srv->fdpoll_method,
		                           cherokee_fdlimit,
		                           fds_per_thread,
		                           conns_per_thread,
		                           keepalive_per_thread);
		if (unlikely(ret < ret_ok)) {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_NEW_THREAD, ret);
			return ret;
		}

		/* Add it to the thread list
		 */
		cherokee_list_add (LIST(thread), &srv->thread_list);
	}
#endif

	return ret_ok;
}


static ret_t
initialize_loggers (cherokee_server_t *srv)
{
	ret_t            ret;
	cherokee_list_t *i;

	/* Initialize the error writers
	 */
	list_for_each (i, &srv->vservers) {
		cherokee_logger_writer_t *writer;

		writer = VSERVER(i)->error_writer;
		if (writer == NULL) {
			continue;
		}

		ret = cherokee_logger_writer_open (writer);
		if (ret != ret_ok) {
			return ret;
		}
	}

	/* Initialize loggers as well
	 */
	list_for_each (i, &srv->vservers) {
		cherokee_logger_t *logger;

		logger = VSERVER(i)->logger;
		if (logger == NULL) {
			continue;
		}

		ret = cherokee_logger_init (logger);
		if (ret != ret_ok) {
			return ret;
		}
	}

	return ret_ok;
}


static ret_t
vservers_check_tls (cherokee_server_t *srv)
{
	ret_t            ret;
	cherokee_list_t *i;
	cuint_t          num = 0;

	list_for_each (i, &srv->vservers) {
		ret = cherokee_virtual_server_has_tls (VSERVER(i));
		if (ret == ret_ok) {
			if (srv->cryptor == NULL) {
				LOG_CRITICAL (CHEROKEE_ERROR_SERVER_NO_CRYPTOR,
				              VSERVER(i)->name.buf);
				return ret_not_found;
			}

			TRACE (ENTRIES, "Virtual Server %s: TLS enabled\n", VSERVER(i)->name.buf);
			return ret_ok;
		}
		num += 1;
	}

	TRACE (ENTRIES, "None of the %d virtual servers use TLS\n", num);
	return ret_not_found;
}


static ret_t
init_vservers_tls (cherokee_server_t *srv)
{
	ret_t               ret;
	cherokee_list_t    *i;
	cuint_t             ok    = 0;
	cherokee_boolean_t  error = false;

	/* Initialize TLS in all the virtual servers
	 */
	list_for_each (i, &srv->vservers) {
		cherokee_virtual_server_t *vserver = VSERVER(i);

		ret = cherokee_virtual_server_init_tls (vserver);
		if (ret < ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_TLS_INIT,
			              cherokee_buffer_is_empty(&vserver->name) ? "unknown" : vserver->name.buf);
			error = true;

		} else if (ret == ret_ok) {
			ok += 1;
		}
	}

	if (error)
		return ret_error;

	/* Ensure 'default' supports TLS/SSL
	 */
	if ((ok > 0) &&
	    (VSERVER(srv->vservers.prev)->cryptor == NULL))
	{
		LOG_CRITICAL_S (CHEROKEE_ERROR_SERVER_TLS_DEFAULT);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
raise_fd_limit (cherokee_server_t *srv, cint_t new_limit)
{
	ret_t ret;

	UNUSED(srv);

	/* Sanity check
	 */
	if (new_limit < FD_NUM_MIN_SYSTEM)
		new_limit = FD_NUM_MIN_SYSTEM;

	/* Set it
	 */
	ret = cherokee_sys_fdlimit_set (new_limit);
	if (ret < ret_ok) {
		LOG_WARNING (CHEROKEE_ERROR_SERVER_FD_SET, new_limit);
	}

	/* Update the new value
	 */
	ret = cherokee_sys_fdlimit_get (&cherokee_fdlimit);
	if (ret < ret_ok) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_SERVER_FD_GET);
		return ret_error;
	}

	return ret_ok;
}

static ret_t
initialize_collectors (cherokee_server_t *srv)
{
	ret_t                      ret;
	cherokee_list_t           *i;
	cherokee_virtual_server_t *vsrv;

	if (srv->collector == NULL) {
		return ret_ok;
	}

	TRACE (ENTRIES, "Initializing the main information collector %s", "\n");

	ret = cherokee_collector_init (srv->collector);
	if (ret != ret_ok) {
		return ret_error;
	}

	list_for_each (i, &srv->vservers) {
		vsrv = VSERVER(i);

		if (vsrv->collector == NULL) {
			continue;
		}

		TRACE (ENTRIES, "Initializing collector for vserver '%s'\n", vsrv->name.buf);

		ret = cherokee_collector_vsrv_init (vsrv->collector, vsrv);
		if (ret != ret_ok) {
			return ret_error;
		}
	}

	return ret_ok;
}

ret_t
cherokee_server_initialize (cherokee_server_t *srv)
{
	int              re;
	ret_t            ret;
	cherokee_list_t *i, *tmp;
	struct passwd    ent;
	char             ent_tmp[1024];


	/* Build the server string
	 */
	cherokee_buffer_add_va (&srv->timeout_header, "Keep-Alive: timeout=%d", srv->timeout);

	/* Set the FD number limit
	 */
	if (srv->fdlimit_custom != -1) {
		ret = raise_fd_limit (srv, srv->fdlimit_custom);
		if (ret < ret_ok)
			return ret;
	}

	/* Verify if there are enough fds.
	 */
	if (cherokee_fdlimit < FD_NUM_MIN_SYSTEM) {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_LOW_FD_LIMIT,
		              cherokee_fdlimit, FD_NUM_MIN_SYSTEM);
		return ret_error;
	}

	/* Set the max number of usable file descriptors
	 * NOTE: fdlimit_available is roughly half of max. system
	 *       limit because for each accepted connection we reserve
	 *       1 spare fd that can be used for opening a file or for
	 *       making a new connection to a backend server
	 *       (i.e. FastCGI, SCGI, mirror, etc.).
	 */
	srv->fdlimit_available = (cherokee_fdlimit - FD_NUM_SPARE);
	if (srv->fdlimit_available < FD_NUM_MIN_AVAILABLE) {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_LOW_FD_LIMIT,
		              srv->fdlimit_available, FD_NUM_MIN_AVAILABLE);
		return ret_error;
	}

	/* Init the SSL/TLS support
	*/
	ret = vservers_check_tls(srv);
	switch (ret) {
		case ret_ok:
			srv->tls_enabled = true;
			break;
		case ret_not_found:
			srv->tls_enabled = false;
			break;
		case ret_error:
			return ret_error;
		default:
			RET_UNKNOWN(ret);
			return ret_error;
	}

	if (srv->tls_enabled) {
		/* Init TLS
		 */
		ret = init_vservers_tls (srv);
		if (ret != ret_ok)
			return ret;
	} else {
		/* Ensure no TLS ports are bound
		 */
		list_for_each_safe (i, tmp, &srv->listeners) {
			if (! BIND_IS_TLS(i))
				continue;

			LOG_WARNING (CHEROKEE_ERROR_SERVER_IGNORE_TLS, BIND(i)->port);
			cherokee_list_del (i);
			cherokee_bind_free (BIND(i));
		}
	}

	/* Ensure there is at least one listener
	*/
	if (cherokee_list_empty (&srv->listeners)) {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_NO_BIND);
		return ret_error;
	}

	/* Initialize the incoming sockets
	 */
	list_for_each (i, &srv->listeners) {
		ret = cherokee_bind_init_port (BIND(i),
		                               srv->listen_queue,
		                               srv->ipv6,
		                               srv->server_token);
		if (ret != ret_ok)
			return ret;
	}

	/* Verify the thread number and force it within sane limits.
	 * See also subsequent fdlimit_per_threads.
	 */
#ifdef HAVE_PTHREAD
	if (srv->thread_num < 1) {
		srv->thread_num = cherokee_cpu_number * 5;
	}

	/* Limit the number of threads
	 * so that each thread has at least 2 fds available.
	 */
	if (srv->thread_num > (srv->fdlimit_available / FD_NUM_MIN_PER_THREAD)) {
		srv->thread_num = (srv->fdlimit_available / FD_NUM_MIN_PER_THREAD);
		if (srv->thread_num < 1)
			srv->thread_num = 1;
	}
#else
	srv->thread_num = 1;
#endif

	/* Check the number of reusable connections
	 */
	if (srv->conns_reuse_max == -1)
		srv->conns_reuse_max = DEFAULT_CONN_REUSE;

	if (srv->conns_reuse_max > srv->fdlimit_available)
		srv->conns_reuse_max = srv->fdlimit_available;

	/* Get the passwd file entry before chroot
	 */
	if ((srv->user != srv->user_orig) ||
	    (srv->group != srv->group_orig))
	{
		ret = cherokee_getpwuid (srv->user, &ent, ent_tmp, sizeof(ent_tmp));
		if (ret != ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_UID_GET, srv->user);
			return ret_error;
		}
	}

	/* Initialize loggers
	 */
	ret = initialize_loggers (srv);
	if (unlikely(ret < ret_ok)) {
		return ret;
	}

	/* Chroot
	 */
	if (! cherokee_buffer_is_empty (&srv->chroot)) {
		/* Jail the process */
		re = chroot (srv->chroot.buf);
		srv->chrooted = (re == 0);
		if (srv->chrooted == 0) {
			LOG_ERRNO (errno, cherokee_err_error,
			           CHEROKEE_ERROR_SERVER_CHROOT, srv->chroot.buf);
			return ret_error;
		}
	}

	/* Change the user
	 */
	if ((srv->user != srv->user_orig) ||
	    (srv->group != srv->group_orig))
	{
		ret = change_execution_user (srv, &ent);
		if (ret != ret_ok) {
			return ret;
		}
	}

	/* Change current directory
	 */
	re = chdir ("/");
	if (re < 0) {
		LOG_ERRNO (errno, cherokee_err_error,
		           CHEROKEE_ERROR_SERVER_CHDIR, "/");
		return ret_error;
	}

	/* Collectors
	 */
	ret = initialize_collectors (srv);
	if (ret != ret_ok)
		return ret_error;

	/* Create the threads
	 */
	ret = initialize_server_threads (srv);
	if (unlikely(ret < ret_ok))
		return ret;

	/* Print the server banner
	 */
	ret = print_banner (srv);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


static void
flush_logs (cherokee_server_t *srv)
{
	cherokee_list_t   *i;
	cherokee_logger_t *logger;

	list_for_each (i, &srv->vservers) {
		logger = VSERVER_LOGGER(i);
		if (logger) {
			cherokee_logger_flush (VSERVER_LOGGER(i));
		}
	}
}


static void
flcaches_cleanup (cherokee_server_t *srv)
{
	cherokee_list_t           *i;
	cherokee_virtual_server_t *vsrv;

	list_for_each (i, &srv->vservers) {
		vsrv = VSERVER(i);

		if (vsrv->flcache == NULL)
			continue;

		cherokee_flcache_cleanup (vsrv->flcache);
	}
}


ret_t
cherokee_server_stop (cherokee_server_t *srv)
{
	if (srv == NULL)
		return ret_ok;

	/* Point it that want to exit
	 */
	srv->wanna_exit = true;

	/* Flush logs
	 */
	flush_logs (srv);

	return ret_ok;
}


ret_t
cherokee_server_unlock_threads (cherokee_server_t *srv)
{
	ret_t            ret;
	cherokee_list_t *i;

	/* Update bogotime before launch the threads
	 */
	cherokee_bogotime_update();

	/* Launch all the threads
	 */
	list_for_each (i, &srv->thread_list) {
		ret = cherokee_thread_unlock (THREAD(i));
		if (unlikely(ret < ret_ok)) return ret;
	}

	return ret_ok;
}


ret_t
cherokee_server_step (cherokee_server_t *srv)
{
	ret_t ret;

	/* Wanna exit ?
	 */
	if (unlikely (srv->wanna_exit)) {
		return ret_eof;
	}

	/* Get the server time.
	 */
	cherokee_bogotime_update();

	/* Execute thread step.
	 */
#ifdef HAVE_PTHREAD
	if (srv->thread_num > 1) {
		ret = cherokee_thread_step_MULTI_THREAD (srv->main_thread, true);
	} else
#endif
	{
		ret = cherokee_thread_step_SINGLE_THREAD (srv->main_thread);
	}

	/* Programmed tasks
	 */
	if (srv->log_flush_next < cherokee_bogonow_now) {
		flush_logs (srv);
		srv->log_flush_next = cherokee_bogonow_now + srv->log_flush_lapse;
	}

	if (srv->nonces_cleanup_next < cherokee_bogonow_now) {
		cherokee_nonce_table_cleanup (srv->nonces);
		srv->nonces_cleanup_next = cherokee_bogonow_now + srv->nonces_cleanup_lapse;
	}

	if (srv->flcache_next < cherokee_bogonow_now) {
		flcaches_cleanup (srv);
		srv->flcache_next = cherokee_bogonow_now + srv->flcache_lapse;
	}

	/* Gracefull restart:
	 */
	if (unlikely ((ret == ret_eof) &&
		      (srv->wanna_reinit)))
	{
		cherokee_list_t *i;

		list_for_each (i, &srv->thread_list) {
			cherokee_thread_wait_end (THREAD(i));
		}

		return ret_eof;
	}

	return ret_eagain;
}


static ret_t
add_source (cherokee_config_node_t *conf, void *data)
{
	ret_t                          ret;
	cherokee_buffer_t             *buf;
	cherokee_source_interpreter_t *src2;
	cint_t                         prio  = -1;
	cherokee_source_t             *src   = NULL;
	cherokee_server_t             *srv   = SRV(data);

	/* Sanity check
	 */
	ret = cherokee_atoi (conf->key.buf, &prio);
	if ((ret != ret_ok) || (prio < 0)) {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_SOURCE, conf->key.buf);
		return ret_error;
	}

	TRACE (ENTRIES, "Adding Source prio=%d\n", prio);

	/* Look for the type of the source objects
	 */
	ret = cherokee_config_node_read (conf, "type", &buf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_SOURCE_TYPE, prio);
		return ret_error;
	}

	if (equal_buf_str (buf, "interpreter")) {
		ret = cherokee_source_interpreter_new (&src2);
		if (ret != ret_ok) return ret;

		ret = cherokee_source_interpreter_configure (src2, conf, prio);
		if (ret != ret_ok) {
			cherokee_source_free (SOURCE(src2));
			return ret;
		}

		src = SOURCE(src2);

	} else if (equal_buf_str (buf, "host")) {
		ret = cherokee_source_new (&src);
		if (ret != ret_ok) return ret;

		ret = cherokee_source_configure (src, conf);
		if (ret != ret_ok) {
			cherokee_source_free (src);
			return ret;
		}

	} else {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_SOURCE_TYPE_UNKNOWN, prio, buf->buf);
		return ret_error;
	}

	cherokee_avl_add (&srv->sources, &conf->key, src);
	return ret_ok;
}

static ret_t
add_vserver (cherokee_config_node_t *conf, void *data)
{
	ret_t                      ret;
	cint_t                     prio = -1;
	cherokee_virtual_server_t *vsrv = NULL;
	cherokee_server_t         *srv  = SRV(data);

	ret = cherokee_atoi (conf->key.buf, &prio);
	if ((ret != ret_ok) || (prio < 0)) {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_VSERVER_PRIO, conf->key.buf);
		return ret_error;
	}

	TRACE (ENTRIES, "Adding vserver prio=%d\n", prio);

	/* Create a new vserver and enqueue it
	 */
	ret = cherokee_virtual_server_new (&vsrv, srv);
	if (ret != ret_ok) return ret;

	/* Configure the new object
	 */
	ret = cherokee_virtual_server_configure (vsrv, prio, conf);
	if (ret == ret_deny) {
		cherokee_virtual_server_free (vsrv);
		return ret_ok;
	} else if (ret != ret_ok) {
		cherokee_virtual_server_free (vsrv);
		return ret;
	}

	/* Add it to the list
	 */
	cherokee_list_add_tail (LIST(vsrv), &srv->vservers);
	return ret_ok;
}


static int
vserver_cmp (cherokee_list_t *a, cherokee_list_t *b)
{
	return (VSERVER(b)->priority - VSERVER(a)->priority);
}

static ret_t
vservers_check_sanity (cherokee_server_t *srv)
{
	cherokee_virtual_server_t *vsrv;
	size_t                     len  = 0;

	/* Ensure that there is at least one vserver
	 */
	cherokee_list_get_len (&srv->vservers, &len);
	if (len == 0) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_SERVER_NO_VSERVERS);
		return ret_error;
	}

	/* Ensure that the lowest priority (last) vserver is 'default'
	 */
	vsrv = VSERVER(srv->vservers.prev);
	if (! equal_buf_str (&vsrv->name, "default")) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_SERVER_NO_DEFAULT_VSERVER);
		return ret_error;
	}

	return ret_ok;
}

static ret_t
configure_bind (cherokee_server_t      *srv,
                cherokee_config_node_t *conf)
{
	ret_t            ret;
	cherokee_list_t *i;
	cherokee_bind_t *listener;

	/* Configure
	 */
	list_for_each (i, &conf->child) {
		ret = cherokee_bind_new (&listener);
		if (ret != ret_ok)
			return ret;

		ret = cherokee_bind_configure (listener, CONFIG_NODE(i));
		if (ret != ret_ok) {
			cherokee_bind_free (listener);
			return ret;
		}

		cherokee_list_add_tail (&listener->listed, &srv->listeners);
	}

	return ret_ok;
}


static ret_t
configure_server_property (cherokee_config_node_t *conf, void *data)
{
	ret_t              ret;
	int                val;
	char              *key = conf->key.buf;
	cherokee_server_t *srv = SRV(data);
	long               num;

	if (equal_buf_str (&conf->key, "fdlimit")) {
		num = strtol (conf->val.buf, NULL, 10);
		srv->fdlimit_custom = MAX(num, cherokee_fdlimit);

	} else if (equal_buf_str (&conf->key, "listen_queue")) {
		ret = cherokee_atoi (conf->val.buf, &srv->listen_queue);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "thread_number")) {
		ret = cherokee_atoi (conf->val.buf, &srv->thread_num);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "sendfile_min")) {
		ret = cherokee_atoi (conf->val.buf, &val);
		if (ret != ret_ok) return ret_error;
		srv->sendfile.min = val;

	} else if (equal_buf_str (&conf->key, "sendfile_max")) {
		ret = cherokee_atoi (conf->val.buf, &val);
		if (ret != ret_ok) return ret_error;
		srv->sendfile.max = val;

	} else if (equal_buf_str (&conf->key, "max_connection_reuse")) {
		ret = cherokee_atoi (conf->val.buf, &srv->conns_reuse_max);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "ipv6")) {
		ret = cherokee_atob (conf->val.buf, &srv->ipv6);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "timeout")) {
		ret = cherokee_atoi (conf->val.buf, &srv->timeout);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "log_flush_lapse")) {
		ret = cherokee_atoi (conf->val.buf, &srv->log_flush_lapse);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "flcache_lapse")) {
		srv->flcache_lapse = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "nonces_cleanup_lapse")) {
		ret = cherokee_atoi (conf->val.buf, &srv->nonces_cleanup_lapse);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "keepalive")) {
		ret = cherokee_atob (conf->val.buf, &srv->keepalive);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "keepalive_max_requests")) {
		ret = cherokee_atoi (conf->val.buf, &val);
		if (ret != ret_ok) return ret_error;
		srv->keepalive_max = val;

	} else if (equal_buf_str (&conf->key, "chunked_encoding")) {
		ret = cherokee_atob (conf->val.buf, &srv->chunked_encoding);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "readable_errors")) {
		ret = cherokee_atob (conf->val.buf, (cherokee_boolean_t *)&cherokee_readable_errors);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "panic_action")) {
		cherokee_buffer_clean (&srv->panic_action);
		cherokee_buffer_add_buffer (&srv->panic_action, &conf->val);

	} else if (equal_buf_str (&conf->key, "chroot")) {
		cherokee_buffer_clean (&srv->chroot);
		cherokee_buffer_add_buffer (&srv->chroot, &conf->val);

	} else if (equal_buf_str (&conf->key, "pid_file")) {
		cherokee_buffer_clean (&srv->pidfile);
		cherokee_buffer_add_buffer (&srv->pidfile, &conf->val);

	} else if (equal_buf_str (&conf->key, "themes_dir")) {
		cherokee_buffer_clean (&srv->themes_dir);
		cherokee_buffer_add_buffer (&srv->themes_dir, &conf->val);

	} else if (equal_buf_str (&conf->key, "bind")) {
		ret = configure_bind (srv, conf);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "poll_method")) {
		char    *str          = conf->val.buf;
		cuint_t  sys_fd_limit = 0;
		cuint_t  thr_fd_limit = 0;

		/* Convert poll method string to type.
		 */
		ret = cherokee_fdpoll_str_to_method(str, &(srv->fdpoll_method));
		if (ret != ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_POLLING_UNKNOWN, str);
			return ret_error;
		}

		/* Verify whether the method is supported by this OS.
		 */
		ret = cherokee_fdpoll_get_fdlimits (srv->fdpoll_method, &sys_fd_limit, &thr_fd_limit);
		switch (ret) {
		case ret_ok:
			break;
		case ret_no_sys:
			LOG_WARNING (CHEROKEE_ERROR_SERVER_POLLING_UNSUPPORTED, str);
			return ret;
		default:
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_POLLING_UNRECOGNIZED, str);
			return ret_error;
		}

	} else if (equal_buf_str (&conf->key, "server_tokens")) {
		if (equal_buf_str (&conf->val, "Product")) {
			srv->server_token = cherokee_version_product;
		} else if (equal_buf_str (&conf->val, "Minor")) {
			srv->server_token = cherokee_version_minor;
		} else if (equal_buf_str (&conf->val, "Minimal")) {
			srv->server_token = cherokee_version_minimal;
		} else if (equal_buf_str (&conf->val, "OS")) {
			srv->server_token = cherokee_version_os;
		} else if (equal_buf_str (&conf->val, "Full")) {
			srv->server_token = cherokee_version_full;
		} else {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_TOKEN, conf->val.buf);
			return ret_error;
		}

	} else if (equal_buf_str (&conf->key, "thread_policy")) {
#ifdef HAVE_PTHREAD
		if (equal_buf_str (&conf->val, "fifo")) {
			srv->thread_policy = SCHED_FIFO;
		} else if (equal_buf_str (&conf->val, "rr")) {
			srv->thread_policy = SCHED_RR;
		} else if (equal_buf_str (&conf->val, "other")) {
			srv->thread_policy = SCHED_OTHER;
		} else {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_THREAD_POLICY, conf->val.buf);
			return ret_error;
		}
#else
		LOG_WARNING (CHEROKEE_ERROR_SERVER_THREAD_IGNORE, conf->val.buf);
#endif

	} else if (equal_buf_str (&conf->key, "user")) {
		struct passwd pwd;
		char          tmp[1024];

		ret = cherokee_getpwnam (conf->val.buf, &pwd, tmp, sizeof(tmp));
		if ((ret != ret_ok) || (pwd.pw_dir == NULL)) {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_USER_NOT_FOUND, conf->val.buf);
			return ret_error;
		}

		srv->user = pwd.pw_uid;

	} else if (equal_buf_str (&conf->key, "group")) {
		struct group grp;
		char         tmp[GRNAM_BUF_LEN];

		ret = cherokee_getgrnam (conf->val.buf, &grp, tmp, sizeof(tmp));
		if (ret != ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_SERVER_GROUP_NOT_FOUND, conf->val.buf);
			return ret_error;
		}

		srv->group = grp.gr_gid;

	} else if (equal_buf_str (&conf->key, "tls")) {
		cryptor_func_new_t      instance;
		cherokee_plugin_info_t *info      = NULL;

		if (! cherokee_buffer_is_empty (&conf->val)) {
			ret = cherokee_plugin_loader_get (&srv->loader, conf->val.buf, &info);
			if ((ret != ret_ok) || (info == NULL))
				return ret;

			instance = (cryptor_func_new_t) info->instance;
			ret = instance ((void **) &srv->cryptor);
			if (ret != ret_ok)
				return ret;

			ret = cherokee_cryptor_configure (srv->cryptor, conf, srv);
			if (ret != ret_ok)
				return ret;
		}

	} else if (equal_buf_str (&conf->key, "collector")) {
		collector_func_new_t    instance;
		cherokee_plugin_info_t *info      = NULL;

		ret = cherokee_plugin_loader_get (&srv->loader, conf->val.buf, &info);
		if ((ret != ret_ok) || (info == NULL))
			return ret;

		instance = (collector_func_new_t) info->instance;
		ret = instance ((void **) &srv->collector, info, conf);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "post_track")) {
		post_track_new_t        instance;
		post_track_configure_t  configure;
		cherokee_plugin_info_t *info      = NULL;

		ret = cherokee_plugin_loader_get (&srv->loader, conf->val.buf, &info);
		if ((ret != ret_ok) || (info == NULL))
			return ret;

		instance = (post_track_new_t) info->instance;
		ret = instance ((void **) &srv->post_track);
		if (ret != ret_ok)
			return ret;

		configure = (post_track_configure_t) info->configure;
		ret = configure ((void **) srv->post_track, conf);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "module_dir") ||
		   equal_buf_str (&conf->key, "module_deps") ||
		   equal_buf_str (&conf->key, "iocache")) {
		/* Ignore it: Previously handled
		 */

	} else {
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_PARSE, key);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
configure_server (cherokee_server_t *srv)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf, *subconf2;

	/* Server
	 */
	TRACE (ENTRIES, "Configuring %s\n", "server");
	ret = cherokee_config_node_get (&srv->config, "server", &subconf);
	if (ret == ret_ok) {
		/* Modules dir and deps
		 */
		ret = cherokee_config_node_get (subconf, "module_dir", &subconf2);
		if (ret == ret_ok) {
			ret = cherokee_plugin_loader_set_directory (&srv->loader, &subconf2->val);
			if (ret != ret_ok) return ret;
		}

		ret = cherokee_config_node_get (subconf, "module_deps", &subconf2);
		if (ret == ret_ok) {
			ret = cherokee_plugin_loader_set_deps_dir (&srv->loader, &subconf2->val);
			if (ret != ret_ok) return ret;
		}

		/* Server properties
		 */
		ret = cherokee_config_node_while (subconf, configure_server_property, srv);
		if (ret != ret_ok) return ret;
	}

	/* Icons
	 */
	TRACE (ENTRIES, "Configuring %s\n", "icons");
	ret = cherokee_config_node_get (&srv->config, "icons", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_icons_new (&srv->icons);
		if (ret != ret_ok) return ret;

		ret =  cherokee_icons_configure (srv->icons, subconf);
		if (ret != ret_ok) return ret;
	}

	/* Mime
	 */
	TRACE (ENTRIES, "Configuring %s\n", "mime");
	ret = cherokee_config_node_get (&srv->config, "mime", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_mime_new (&srv->mime);
		if (ret != ret_ok) return ret;

		ret = cherokee_mime_configure (srv->mime, subconf);
		if (ret != ret_ok) return ret;
	}

	/* Sources
	 */
	TRACE (ENTRIES, "Configuring %s\n", "sources");
	ret = cherokee_config_node_get (&srv->config, "source", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_config_node_while (subconf, add_source, srv);
		if (ret != ret_ok) return ret;
	}

	/* IO-cache
	 */
	TRACE (ENTRIES, "Configuring %s\n", "iocache");
	ret = cherokee_config_node_read_bool (&srv->config, "server!iocache", &srv->iocache_enabled);

	if ((ret == ret_ok) && (srv->iocache_enabled)) {
		ret = cherokee_iocache_new (&srv->iocache);
		if (ret != ret_ok)
			return ret;

		ret = cherokee_config_node_get (&srv->config, "server!iocache", &subconf);
		if (ret == ret_ok) {
			ret = cherokee_iocache_configure (srv->iocache, subconf);
			if (ret != ret_ok)
				return ret;
		}
	}

	/* Load the virtual servers
	 */
	TRACE (ENTRIES, "Configuring %s\n", "virtual servers");
	ret = cherokee_config_node_get (&srv->config, "vserver", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_config_node_while (subconf, add_vserver, srv);
		if (ret != ret_ok) return ret;
	}

	cherokee_list_sort (&srv->vservers, vserver_cmp);

	/* Sanity check: virtual servers
	 */
	ret = vservers_check_sanity (srv);
	if (ret != ret_ok)
		return ret;

	/* Sanity check: Port Binds
	 */
	if (cherokee_list_empty (&srv->listeners)) {
		cherokee_bind_t *listener;

		ret = cherokee_bind_new (&listener);
		if (ret != ret_ok)
			return ret;

		ret = cherokee_bind_set_default (listener);
		if (ret != ret_ok)
			return ret;

		cherokee_list_add (&listener->listed, &srv->listeners);
	}

	return ret_ok;
}


ret_t
cherokee_server_read_config_string (cherokee_server_t *srv, cherokee_buffer_t *string)
{
	ret_t ret;

	/* Load the main file
	 */
	ret = cherokee_config_reader_parse_string (&srv->config, string);
	if (ret != ret_ok) return ret;

	ret = configure_server (srv);
	if (ret != ret_ok) return ret;

	/* Clean up
	 */
	ret = cherokee_config_node_mrproper (&srv->config);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


ret_t
cherokee_server_read_config_file (cherokee_server_t *srv, const char *fullpath)
{
	ret_t             ret;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	cherokee_buffer_add (&tmp, fullpath, strlen(fullpath));

	/* Load the main file
	 */
	ret = cherokee_config_reader_parse (&srv->config, &tmp);
	if (ret != ret_ok) goto error;

	ret = configure_server (srv);
	if (ret != ret_ok) goto error;

	/* Clean up
	 */
	ret = cherokee_config_node_mrproper (&srv->config);
	if (ret != ret_ok) goto error;

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&tmp);
	return ret;
}


ret_t
cherokee_server_daemonize (cherokee_server_t *srv)
{
	pid_t child_pid;

	TRACE (ENTRIES, "server (%p) about to become evil", srv);

	child_pid = fork();
	switch (child_pid) {
	case -1:
		LOG_CRITICAL_S (CHEROKEE_ERROR_SERVER_FORK);
		break;

	case 0:
		close(2);
		close(1);
		close(0);

		setsid();
		break;

	default:
		exit(0);
	}

	UNUSED(srv);
	return ret_ok;
}


ret_t
cherokee_server_get_conns_num (cherokee_server_t *srv, cuint_t *num)
{
	cuint_t          conns_num = 0;
	cherokee_list_t *thread;

	/* Open HTTP connections number
	 */
	list_for_each (thread, &srv->thread_list) {
		conns_num += THREAD(thread)->conns_num;
	}

	conns_num += srv->main_thread->conns_num;

	/* Return out parameters
	 */
	*num = conns_num;
	return ret_ok;
}


ret_t
cherokee_server_get_active_conns (cherokee_server_t *srv, cuint_t *num)
{
	cuint_t          active = 0;
	cherokee_list_t *thread;

	/* Active connections number
	 */
	list_for_each (thread, &srv->thread_list) {
		active += THREAD(thread)->active_list_num;
	}

	active += srv->main_thread->active_list_num;

	/* Return out parameters
	 */
	*num = active;

	return ret_ok;
}


ret_t
cherokee_server_get_reusable_conns (cherokee_server_t *srv, cuint_t *num)
{
	cuint_t          reusable = 0;
	cherokee_list_t *thread;

	/* Reusable connections
	 */
	list_for_each (thread, &srv->thread_list) {
		reusable += THREAD(thread)->reuse_list_num;
	}
	reusable += srv->main_thread->reuse_list_num;

	/* Return out parameters
	 */
	*num = reusable;
	return ret_ok;
}


ret_t
cherokee_server_handle_HUP (cherokee_server_t *srv)
{
	cherokee_list_t *i;

	srv->wanna_reinit     = true;
	srv->keepalive        = false;
	srv->keepalive_max    = 0;

	list_for_each (i, &srv->listeners) {
		/* Do not call cherokee_socket_close(). It'd close the
		 * fd and set it to -1. If a thread added it to its
		 * fdpoll, it couldn't take the fd out of the poll.
		 */
		TRACE (ENTRIES, "Closing listener fd=%d\n", BIND(i)->socket.socket);
		cherokee_fd_close (BIND(i)->socket.socket);
	}


	return ret_ok;
}


ret_t
cherokee_server_handle_TERM (cherokee_server_t *srv)
{
	if (srv != NULL) {
		srv->wanna_exit = true;
	}

	return ret_ok;
}


NORETURN void
cherokee_server_handle_panic (cherokee_server_t *srv)
{
	int               re;
	cherokee_buffer_t cmd = CHEROKEE_BUF_INIT;

	PRINT_MSG_S ("Cherokee feels panic!\n");

	if ((srv == NULL) || (srv->panic_action.len <= 0)) {
		goto fin;
	}

	cherokee_buffer_add_va (&cmd, "%s %d", srv->panic_action.buf, getpid());

	re = system (cmd.buf);
	if (re < 0) {
#ifdef WEXITSTATUS
		int val = WEXITSTATUS(re);
#else
		int val = re;
#endif
		LOG_CRITICAL (CHEROKEE_ERROR_SERVER_PANIC, cmd.buf, val);
	}

	cherokee_buffer_mrproper (&cmd);
fin:
	abort();
}

static ret_t
list_find_connection (cherokee_list_t *list, culong_t id) {
	cherokee_list_t *c;

	list_for_each (c, list) {
		cherokee_connection_t *conn = CONN(c);

		if (conn->id == id) {
			if ((conn->phase != phase_nothing) &&
			    (conn->phase != phase_lingering))
			{
				conn->phase = phase_shutdown;
				return ret_ok;
			}
		}
	}

	return ret_not_found;
}

static ret_t
thread_find_connection (cherokee_thread_t *thread, culong_t id) {
	ret_t ret;

	ret = list_find_connection (&thread->active_list, id);
	if (ret == ret_ok) return ret_ok;

	ret = list_find_connection (&thread->polling_list, id);
	if (ret == ret_ok) return ret_ok;

	return list_find_connection (&thread->limiter.conns, id);
}

ret_t
cherokee_server_close_connection (cherokee_server_t *srv, cherokee_thread_t *mythread, char *id_str)
{
	ret_t ret;
	culong_t         id;
	cherokee_list_t *t;

	id = strtol (id_str, NULL, 10);

	if (srv->main_thread != mythread) CHEROKEE_MUTEX_LOCK (&srv->main_thread->ownership);

	ret = thread_find_connection (srv->main_thread, id);
	if (ret == ret_ok) return ret;

	if (srv->main_thread != mythread) CHEROKEE_MUTEX_UNLOCK (&srv->main_thread->ownership);

	list_for_each (t, &srv->thread_list) {
		cherokee_thread_t *thread = THREAD(t);

		if (thread != mythread) CHEROKEE_MUTEX_LOCK (&thread->ownership);

		ret = thread_find_connection (thread, id);

		if (thread != mythread) CHEROKEE_MUTEX_UNLOCK (&thread->ownership);

		if (ret == ret_ok) return ret;
	}

	return ret_not_found;
}


ret_t
cherokee_server_log_reopen (cherokee_server_t *srv)
{
	ret_t            ret;
	cherokee_list_t *i;

	list_for_each (i, &srv->vservers) {
		cherokee_logger_t *logger = VSERVER(i)->logger;

		if (logger == NULL)
			continue;

		ret = cherokee_logger_reopen (logger);
		if (unlikely (ret != ret_ok)) return ret;
	}

	return ret_ok;
}


ret_t
cherokee_server_set_backup_mode (cherokee_server_t *srv, cherokee_boolean_t active)
{
	ret_t            ret;
	cherokee_list_t *i;

	list_for_each (i, &srv->vservers) {
		cherokee_logger_t *logger = VSERVER(i)->logger;

		if (logger == NULL)
			continue;

		ret = cherokee_logger_set_backup_mode (logger, active);
		if (unlikely (ret != ret_ok)) return ret;
	}

	return ret_ok;
}


ret_t
cherokee_server_get_backup_mode (cherokee_server_t *srv, cherokee_boolean_t *active)
{
	cherokee_list_t *i;

	*active = false;
	list_for_each (i, &srv->vservers) {
		cherokee_logger_t *logger = VSERVER(i)->logger;

		if (logger == NULL)
			continue;

		cherokee_logger_get_backup_mode (logger, active);
		if (*active == true) return ret_ok;
	}

	return ret_ok;
}

ret_t
cherokee_server_get_vserver (cherokee_server_t          *srv,
                             cherokee_buffer_t          *host,
                             cherokee_connection_t      *conn,
                             cherokee_virtual_server_t **vsrv)
{
	int                        re;
	ret_t                      ret;
	cherokee_list_t           *i;
	cherokee_virtual_server_t *vserver;

	TRACE (ENTRIES, "Trying to match '%s'\n", host->buf);

	/* Evaluate the vrules
	 */
	list_for_each (i, &srv->vservers) {
		vserver = VSERVER(i);

		if (! vserver->matching)
			continue;

		ret = cherokee_vrule_match (vserver->matching, host, conn);
		if (ret == ret_ok) {
			TRACE (ENTRIES, "Virtual server '%s' matched vrule\n", vserver->name.buf);
			*vsrv = vserver;
			return ret_ok;
		}
	}

	/* In case there was no match, try with the nicknames
	 */
	list_for_each (i, &srv->vservers) {
		vserver = VSERVER(i);

		if (! vserver->match_nick)
			continue;

		re = cherokee_buffer_cmp_buf (host, &vserver->name);
		if (re == 0) {
			TRACE (ENTRIES, "Virtual server '%s' matched by its nick\n", vserver->name.buf);
			*vsrv = vserver;
			return ret_ok;
		}
	}

	/* Nothing matched, return the 'default' vserver (lowest priority)
	 */
	*vsrv = VSERVER(srv->vservers.prev);

	TRACE (ENTRIES, "No VServer matched, returning '%s'\n", (*vsrv)->name.buf);
	return ret_ok;
}


ret_t
cherokee_server_get_next_bind (cherokee_server_t  *srv,
                               cherokee_bind_t    *bind,
                               cherokee_bind_t   **next)
{
	if (bind->listed.next == &srv->listeners) {
		*next = BIND(srv->listeners.next);
	} else {
		*next = BIND(bind->listed.next);
	}

	return ret_ok;
}


ret_t
cherokee_server_get_log_writer (cherokee_server_t         *srv,
                                cherokee_config_node_t    *config,
                                cherokee_logger_writer_t **writer)
{
	ret_t              ret;
	cherokee_buffer_t  tmp  = CHEROKEE_BUF_INIT;

	/* Build the index name
	 */
	ret = cherokee_logger_writer_get_id (config, &tmp);
	if (ret != ret_ok) {
		goto error;
	}

	/* Check the writers tree
	 */
	ret = cherokee_avl_get (&srv->logger_writers_index, &tmp, (void **)writer);
	if ((ret == ret_ok) && (*writer != NULL)) {
		TRACE(ENTRIES",log", "Reusing logger: '%s'\n", tmp.buf);
		goto ok;
	}

	/* Create a new writer object
	 */
	ret = cherokee_logger_writer_new (writer);
	if (ret != ret_ok) {
		goto error;
	}

	ret = cherokee_logger_writer_configure (*writer, config);
	if (ret != ret_ok) {
		goto error;
	}

	/* Add it to the index
	 */
	cherokee_list_add (&(*writer)->listed, &srv->logger_writers);

	ret = cherokee_avl_add (&srv->logger_writers_index, &tmp, *writer);
	if (ret != ret_ok) {
		goto error;
	}

ok:
	TRACE(ENTRIES",log", "Instanced a new logger: '%s'\n", tmp.buf);

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&tmp);
	return ret_error;
}
