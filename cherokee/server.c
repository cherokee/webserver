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
#include "server-protected.h"
#include "server.h"

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

#ifdef HAVE_GNUTLS
# include <gnutls/gnutls.h>	
# include <gcrypt.h>
#endif

#include <signal.h>
#include <dirent.h>
#include <unistd.h>

#include "thread.h"
#include "socket.h"
#include "connection.h"
#include "mime.h"
#include "util.h"
#include "dtm.h"
#include "fdpoll.h"
#include "fdpoll-protected.h"
#include "regex.h"
#include "fcgi_manager.h"
#include "connection-protected.h"
#include "nonce.h"
#include "config_reader.h"
#include "init.h"

#define ENTRIES "core,server"

/* Number of listen fds (HTTP + HTTPS)
 */
#define MAX_LISTEN_FDS 2

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
	cherokee_socket_init (&n->socket);
	cherokee_socket_init (&n->socket_tls);

	cherokee_buffer_init (&n->unix_socket);

	n->ipv6            = true;
	n->fdpoll_method   = cherokee_poll_UNSET;

	/* Mime types
	 */
	n->mime            = NULL;

	/* Exit related
	 */
	n->wanna_exit      = false;
	n->wanna_reinit    = false;
	n->reinit_callback = NULL;
	
	/* Server config
	 */
	n->port            = 80;
	n->port_tls        = 443;
	n->tls_enabled     = false;

	n->fdwatch_msecs   = 999;

	n->start_time      = time(NULL);

	n->keepalive       = true;
	n->keepalive_max   = MAX_KEEPALIVE;

	n->thread_num      = -1;
	n->thread_policy   = -1;

	n->chrooted        = false;
	n->user_orig       = getuid();
	n->user            = n->user_orig;
	n->group_orig      = getgid();
	n->group           = n->group_orig;

	n->timeout         = 15;

	n->fdlimit_custom      = -1;
	n->fdlimit_available   = -1;
	n->fdlimit_per_thread  = -1;

	n->conns_max           =  0;
	n->conns_reuse_max     = -1;
	n->conns_keepalive_max =  0;
	n->conns_num_bogo      =  0;

	n->listen_queue    = 1024;
	n->sendfile.min    = 32768;
	n->sendfile.max    = 2147483647;

	n->icons           = NULL;
	n->regexs          = NULL;
	n->iocache         = NULL;

	cherokee_buffer_init (&n->listen_to);
	cherokee_buffer_init (&n->chroot);

	cherokee_buffer_init (&n->panic_action);
	cherokee_buffer_add_str (&n->panic_action, CHEROKEE_PANIC_PATH);

	/* Bogo now
	 */
	n->bogo_now        = 0;
	n->bogo_now_tzloc_sign = '+';
	n->bogo_now_tzloc_offset = 0;
	CHEROKEE_RWLOCK_INIT (&n->bogo_now_mutex, NULL);

	CHEROKEE_RWLOCK_WRITER (&n->bogo_now_mutex);
	cherokee_buffer_init (&n->bogo_now_strgmt);
	cherokee_buffer_ensure_size (&n->bogo_now_strgmt, DTM_SIZE_GMTTM_STR);
	CHEROKEE_RWLOCK_UNLOCK (&n->bogo_now_mutex);

	/* Time managing hack
	 */
	cherokee_buffer_init (&n->timeout_header);
	cherokee_buffer_add_str (&n->timeout_header, "Keep-Alive: timeout=15"CRLF);

	/* Accepting mutexes
	 */
#ifdef HAVE_TLS
	CHEROKEE_MUTEX_INIT (&n->accept_tls_mutex, NULL);
#endif
	CHEROKEE_MUTEX_INIT (&n->accept_mutex, NULL);

	/* IO Cache cache
	 */
	cherokee_iocache_new_default (&n->iocache, n);
	return_if_fail (n->iocache != NULL, ret_nomem);	
	n->iocache_clean_next = 0;

	/* Regexs
	 */
	cherokee_regex_table_new (&n->regexs);
	return_if_fail (n->regexs != NULL, ret_nomem);	

	/* Active nonces
	 */
	ret = cherokee_nonce_table_new (&n->nonces);
	if (unlikely(ret < ret_ok)) return ret;	

	/* Module loader
	 */
	cherokee_plugin_loader_init (&n->loader);

	/* Virtual servers list
	 */
	INIT_LIST_HEAD (&n->vservers);

	cherokee_virtual_server_new (&n->vserver_default, n);
	return_if_fail (n->vserver_default!=NULL, ret_nomem);
		
	/* Encoders 
	 */
	cherokee_encoder_table_init (&n->encoders);

	/* Server string
	 */
	n->server_token = cherokee_version_full;
	cherokee_buffer_init (&n->server_string);
	cherokee_buffer_init (&n->server_string_ext);
	cherokee_buffer_init (&n->server_string_w_port);
	cherokee_buffer_init (&n->server_string_w_port_tls);

	/* Loggers
	 */
	n->log_flush_next   = 0;
	n->log_flush_elapse = 10;

	/* TLS
	 */
	ret = cherokee_tls_init();
	if (ret < ret_ok) {
		return ret;
	}

	/* PID
	 */
	cherokee_buffer_init (&n->pidfile);

	/* Config
	 */
	cherokee_config_node_init (&n->config);
	
	/* Return the object
	 */
	*srv = n;
	return ret_ok;
}


static void
close_all_connections (cherokee_server_t *srv)
{
	cherokee_list_t *i;

	cherokee_thread_close_all_connections (srv->main_thread);
	
	list_for_each (i, &srv->thread_list) {
		cherokee_thread_close_all_connections (THREAD(i));
	}
}


static void
free_virtual_servers (cherokee_server_t *srv)
{
	cherokee_list_t *i, *j;

	list_for_each_safe (i, j, &srv->vservers) {
		cherokee_virtual_server_free (VSERVER(i));
	}
}


static ret_t
destroy_thread (cherokee_thread_t *thread)
{
	cherokee_thread_wait_end (thread);
	cherokee_thread_free (thread);

	return ret_ok;
}


static ret_t
destroy_all_threads (cherokee_server_t *srv)
{
	cherokee_list_t *i, *tmp;

	/* Set the exit flag, and try to ensure the threads are not
	 * locked on a semaphore
	 */
	list_for_each_safe (i, tmp, &srv->thread_list) {
		THREAD(i)->exit = true;
		CHEROKEE_MUTEX_UNLOCK (&srv->accept_mutex);
#ifdef HAVE_TLS
		CHEROKEE_MUTEX_UNLOCK (&srv->accept_tls_mutex);
#endif
	}

	/* Destroy the thread object
	 */
	list_for_each_safe (i, tmp, &srv->thread_list) {
		destroy_thread (THREAD(i));
	}

	/* Main thread
	 */
	return cherokee_thread_free (srv->main_thread);
}


ret_t
cherokee_server_free (cherokee_server_t *srv)
{
	/* Threads
	 */
	destroy_all_threads (srv);

	/* File descriptors
	 */
	cherokee_buffer_mrproper (&srv->unix_socket);

	cherokee_socket_close (&srv->socket);
	cherokee_socket_mrproper (&srv->socket);


#ifdef HAVE_TLS
	cherokee_socket_close (&srv->socket);
	cherokee_socket_mrproper (&srv->socket);

	CHEROKEE_MUTEX_DESTROY (&srv->accept_tls_mutex);
#endif
	CHEROKEE_MUTEX_DESTROY (&srv->accept_mutex);

	/* Attached objects
	 */
	cherokee_encoder_table_mrproper (&srv->encoders);

	cherokee_mime_free (srv->mime);
	cherokee_icons_free (srv->icons);
	cherokee_regex_table_free (srv->regexs);
	cherokee_iocache_free_default (srv->iocache);

	cherokee_nonce_table_free (srv->nonces);
	
	/* Virtual servers
	 */
	free_virtual_servers (srv);

	cherokee_virtual_server_free (srv->vserver_default);
	srv->vserver_default = NULL;

	cherokee_buffer_mrproper (&srv->bogo_now_strgmt);
	cherokee_buffer_mrproper (&srv->timeout_header);

	cherokee_buffer_mrproper (&srv->server_string);
	cherokee_buffer_mrproper (&srv->server_string_ext);
	cherokee_buffer_mrproper (&srv->server_string_w_port);
	cherokee_buffer_mrproper (&srv->server_string_w_port_tls);

	cherokee_buffer_mrproper (&srv->listen_to);
	cherokee_buffer_mrproper (&srv->chroot);
	cherokee_buffer_mrproper (&srv->pidfile);
	cherokee_buffer_mrproper (&srv->panic_action);

	/* Module loader: It must be the last action to be performed
	 * because it will close all the opened modules.
	 */
	cherokee_plugin_loader_mrproper (&srv->loader);

	free (srv);	
	return ret_ok;
}


static ret_t
change_execution_user (cherokee_server_t *srv, struct passwd *ent)
{
	int error;

#if 0
	/* Get user information
	*/
	ent = getpwuid (srv->user);
	if (ent == NULL) {
		PRINT_ERROR ("Can't get username for UID %d\n", srv->user);
		return ret_error;
	}
#endif

	/* Reset `groups' attributes.
	 */
	if (srv->user_orig == 0) {
		error = initgroups (ent->pw_name, srv->group);
		if (error == -1) {
			PRINT_ERROR ("initgroups: Unable to set groups for user `%s' and GID %d\n", 
				     ent->pw_name, srv->group);
		}
	}

	/* Change of group requested
	 */
	if (srv->group != srv->group_orig) {
		error = setgid (srv->group);
		if (error != 0) {
			PRINT_ERROR ("Can't change group to GID %d, running with GID=%d\n",
				     srv->group, srv->group_orig);
		}
	}

	/* Change of user requested
	 */
	if (srv->user != srv->user_orig) {
		error = setuid (srv->user);		
		if (error != 0) {
			PRINT_ERROR ("Can't change user to UID %d, running with UID=%d\n",
				     srv->user, srv->user_orig);
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
set_server_fd_socket_opts (int socket)
{
	int           re;
	int           on;
        struct linger ling = {0, 0};

	/* Set 'close-on-exec'
	 */
	CLOSE_ON_EXEC(socket);
		
	/* To re-bind without wait to TIME_WAIT
	 */
	on = 1;
	re = setsockopt (socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (re != 0) return ret_error;

	/* TCP_MAXSEG:
	 * The maximum size of a TCP segment is based on the network MTU for des-
	 * tinations on local networks or on a default MTU of 576 bytes for desti-
	 * nations on nonlocal networks.  The default behavior can be altered by
	 * setting the TCP_MAXSEG option to an integer value from 1 to 65,535.
	 * However, TCP will not use a maximum segment size smaller than 32 or
	 * larger than the local network MTU.  Setting the TCP_MAXSEG option to a
	 * value of zero results in default behavior.  The TCP_MAXSEG option can
	 * only be set prior to calling listen or connect on the socket.  For pas-
	 * sive connections, the TCP_MAXSEG option value is inherited from the
	 * listening socket. This option takes an int value, with a range of 0 to
	 * 65535.
	 */
#ifdef TCP_MAXSEG
	on = 64000;
	setsockopt (socket, SOL_SOCKET, TCP_MAXSEG, &on, sizeof(on));

	/* Do no check the returned value */
#endif	

	/* SO_LINGER
	 * kernels that map pages for IO end up failing if the pipe is full
         * at exit and we take away the final buffer.  this is really a kernel
         * bug but it's harmless on systems that are not broken, so...
	 *
	 * http://www.apache.org/docs/misc/fin_wait_2.html
         */
	setsockopt (socket, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

	/* TCP_DEFER_ACCEPT:
	 * Allows a listener to be awakened only when data arrives on the socket.
	 * Takes an integer value (seconds), this can bound the maximum number of
	 * attempts TCP will make to complete the connection. This option should 
	 * not be used in code intended to be portable.
	 *
	 * Give clients 20s to send first data packet 
	 */
#if defined (TCP_DEFER_ACCEPT) && defined(SOL_TCP)
	on = 20;
	re = setsockopt (socket, SOL_TCP, TCP_DEFER_ACCEPT, &on, sizeof(on));
	if (re != 0) return ret_error;
#endif

	/* TODO:
	 * It could be good to add support for the FreeBSD accept filters:
	 * http://www.freebsd.org/cgi/man.cgi?query=accf_http
	 */

	return ret_ok;
}


static ret_t
initialize_server_socket4 (cherokee_server_t *srv, cherokee_socket_t *sock, unsigned short port)
{
	ret_t ret;

	/* Create the socket, and set its properties
	 */
	ret = cherokee_socket_set_client (sock, AF_INET);
	if (ret != ret_ok) return ret;

	ret = set_server_fd_socket_opts (SOCKET_FD(sock));
	if (ret != ret_ok) return ret;

	/* Bind the socket
	 */
	ret = cherokee_socket_bind (sock, port, &srv->listen_to);
	if (ret != ret_ok) return ret;
	
	return ret_ok;
}


static ret_t
initialize_server_socket6 (cherokee_server_t *srv, cherokee_socket_t *sock, unsigned short port)
{
#ifndef HAVE_IPV6
	return ret_no_sys;
#else
	ret_t ret;

	/* Create the socket, and set its properties
	 */
	ret = cherokee_socket_set_client (sock, AF_INET6);
	if (ret != ret_ok) return ret;

	ret = set_server_fd_socket_opts (SOCKET_FD(sock));
	if (ret != ret_ok) return ret;

	/* Bind the socket
	 */
	ret = cherokee_socket_bind (sock, port, &srv->listen_to);
	if (ret != ret_ok) return ret;
	
	return ret_ok;
#endif
}


static ret_t
initialize_server_socket_unix (cherokee_server_t *srv, cherokee_socket_t *sock)
{
#ifndef AF_LOCAL
	return ret_no_sys;
#else
	ret_t ret;

	/* Create the socket, and set its properties
	 */
	ret = cherokee_socket_set_client (sock, AF_LOCAL);
	if (ret != ret_ok) return ret;

	/* Bind the socket
	 */
	ret = cherokee_socket_bind (sock, -1, &srv->unix_socket);
	if (ret != ret_ok) return ret;
	
	return ret_ok;
#endif
}

static ret_t
print_banner (cherokee_server_t *srv)
{
	char             *p;
	char             *method;
	cherokee_buffer_t n = CHEROKEE_BUF_INIT;

	/* First line
	 */
	cherokee_buffer_add_va (&n, "Cherokee Web Server %s (%s): ", PACKAGE_VERSION, __DATE__);

	if (cherokee_socket_configured (&srv->socket) &&
	    cherokee_socket_configured (&srv->socket_tls)) {
		cherokee_buffer_add_va (&n, "Listening on ports %d and %d", srv->port, srv->port_tls);
	} else {
		if (cherokee_socket_configured (&srv->socket)) {
			if (cherokee_buffer_is_empty (&srv->unix_socket))
				cherokee_buffer_add_va (&n, "Listening on port %d", srv->port);
			else
				cherokee_buffer_add_va (&n, "Listening on %s", srv->unix_socket.buf);
		} else 
			cherokee_buffer_add_va (&n, "Listening on port %d", srv->port_tls);
	}

	if (srv->chrooted) {
		cherokee_buffer_add_str (&n, ", chrooted");
	}

	/* TLS / SSL
	 */
	if (srv->tls_enabled) {
#ifdef HAVE_GNUTLS
		cherokee_buffer_add_str (&n, ", with TLS support via GNUTLS");
#elif HAVE_OPENSSL
		cherokee_buffer_add_str (&n, ", with TLS support via OpenSSL");
#endif
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

	/* Threading stuff
	 */
	if (srv->thread_num <= 1) {
		cherokee_buffer_add_str (&n, ", single thread");
		cherokee_buffer_add_va (&n, ", %d fds per thread", srv->fdlimit_per_thread);	
	} else {
		cherokee_buffer_add_va (&n, ", %d threads", srv->thread_num);
		cherokee_buffer_add_va (&n, ", %d fds per thread", srv->fdlimit_per_thread);	

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

	/* Print it!
	 */
	for (p = n.buf+TERMINAL_WIDTH; p < n.buf+n.len; p+=75) {
		while (*p != ',') p--;
		*p = '\n';
	}

	printf ("%s\n", n.buf);
	fflush (stdout);
	cherokee_buffer_mrproper (&n);
	
	return ret_ok;
}


static ret_t
initialize_server_socket (cherokee_server_t *srv, cherokee_socket_t *socket, unsigned short port)
{
	ret_t ret;

	/* Initialize the socket
	 */
	ret = ret_not_found;

#ifdef AF_LOCAL
	if (! cherokee_buffer_is_empty (&srv->unix_socket)) {
		ret = initialize_server_socket_unix (srv, socket);
	}
#endif

#ifdef HAVE_IPV6
	if (srv->ipv6 && (ret != ret_ok)) {
		ret = initialize_server_socket6 (srv, socket, port);
	}
#else
	ret = ret_not_found;
#endif

	if (ret != ret_ok) {
		ret = initialize_server_socket4 (srv, socket, port);

		if (ret != ret_ok) {
			if (! cherokee_buffer_is_empty (&srv->unix_socket)) 
				PRINT_ERROR ("Can't bind() socket (unix=%s, UID=%d, GID=%d)\n", 
					     srv->unix_socket.buf, getuid(), getgid());
			else
				PRINT_ERROR ("Can't bind() socket (port=%d, UID=%d, GID=%d)\n", 
					     port, getuid(), getgid());
			return ret_error;
		}
	}
	   
	/* Set no-delay mode
	 */
	ret = cherokee_socket_set_nodelay (socket);
	if (ret != ret_ok) return ret;

	/* Listen
	 */
	ret = cherokee_socket_listen (socket, srv->listen_queue);
	if (ret != ret_ok) {
		cherokee_socket_close (socket);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
initialize_server_threads (cherokee_server_t *srv)
{	
	ret_t   ret;
	cint_t  i;
	cuint_t fds_per_thread;
	cuint_t fds_per_thread1;
	cuint_t conns_per_thread;

#ifdef HAVE_PTHREAD
	cuint_t thr_fds;
	cuint_t spare_fds;
#endif

	/* Reset max. conns value
	 */
	srv->conns_max           = 0;
	srv->conns_num_bogo      = 0;
	srv->conns_keepalive_max = 0;

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

	fds_per_thread1 = fds_per_thread = srv->fdlimit_available / srv->thread_num;

#ifdef HAVE_PTHREAD
	thr_fds = fds_per_thread * srv->thread_num;
	spare_fds = srv->fdlimit_available - thr_fds;

	/* Add a couple more fds to this thread,
	 * this is useful if we are using a huge number of threads
	 * (i.e. 1000 or more) and we don't want to leave lots of
	 * unused fds.
	 */
	if (spare_fds >= 2) {
		spare_fds -= 2;
		fds_per_thread1 += 2;
	}
#else
	fds_per_thread1 -= MAX_LISTEN_FDS;
#endif
	/* Get fdpoll limits.
	 */
	if (srv->fdpoll_method != cherokee_poll_UNSET) {
		cuint_t sys_fd_limit  = 0;
		cuint_t poll_fd_limit = 0;

		ret = cherokee_fdpoll_get_fdlimits (srv->fdpoll_method, &sys_fd_limit, &poll_fd_limit);
		if (ret != ret_ok) {
			PRINT_ERROR ("cherokee_fdpoll_get_fdlimits: failed %d (poll_type %d)\n", (int)ret, (int) srv->fdpoll_method);
			return ret_error;
		}

		/* Test system fd limit (no autotune here).
		 */
		if ((sys_fd_limit > 0) &&
		    (cherokee_fdlimit > sys_fd_limit)) {
			PRINT_ERROR ("system_fd_limit %d > %d sys_fd_limit\n", cherokee_fdlimit, sys_fd_limit);
			return ret_error;
		}

		/* If polling set limit has too many fds,
		 * then decrease that number.
		 */
		if (poll_fd_limit > 0 &&
		    fds_per_thread1 > poll_fd_limit) {
			PRINT_ERROR ("fds_per_thread1 %d > %d poll_fd_limit (reduce that limit)\n", fds_per_thread1, poll_fd_limit);
			fds_per_thread1 = poll_fd_limit - MAX_LISTEN_FDS;
		}
	}

	/* Max. number of connections is halved because opening a new file
	 * or using a socket to connect to an external helper
	 * might be required to satisfy a request coming from an accepted
	 * and thus already open connection.
	 */
	conns_per_thread = fds_per_thread1 / 2;
	srv->conns_max += conns_per_thread;
	fds_per_thread1 += MAX_LISTEN_FDS;

	/* Set mean fds per thread.
	 */
	srv->fdlimit_per_thread = fds_per_thread1;

	/* Create the main thread (only structures, not a real thread)
	 */
	ret = cherokee_thread_new (&srv->main_thread, srv, thread_sync, 
			srv->fdpoll_method, cherokee_fdlimit,
			fds_per_thread1, conns_per_thread);
	if (unlikely(ret < ret_ok)) {
		PRINT_ERROR("cherokee_thread_new (main_thread) failed %d\n", ret);
		return ret;
	}

	/* If Cherokee is compiled in single thread mode, it has to
	 * add the server socket to the fdpoll of the sync thread
	 */
	if (srv->thread_num == 1) {
		ret = cherokee_thread_accept_on (srv->main_thread);
		if (unlikely(ret < ret_ok)) {
			PRINT_ERROR("cherokee_thread_accept_on failed %d\n", ret);
			return ret;
		}
	}

	/* If Cherokee has been compiled in multi-thread mode,
	 * then it may need to launch other threads.
	 */
#ifdef HAVE_PTHREAD
	for (i = 0; i < srv->thread_num - 1; i++) {
		cherokee_thread_t *thread;

		/* Add a couple more fds to this thread,
		 * this is useful if we are using a huge number of threads
		 * (i.e. 1000 or more) and we don't want to leave lots of
		 * unused fds.
		 */
		fds_per_thread1 = fds_per_thread;
		if (spare_fds >= 2) {
			spare_fds -= 2;
			fds_per_thread1 += 2;
		}
		conns_per_thread = fds_per_thread1 / 2;
		srv->conns_max += conns_per_thread;
		fds_per_thread1 += MAX_LISTEN_FDS;

		/* NOTE: mean fds per thread has already been set above.
		 */

		/* Create a real thread.
		 */
		ret = cherokee_thread_new (&thread, srv, thread_async, 
				srv->fdpoll_method, cherokee_fdlimit,
				fds_per_thread1, conns_per_thread);
		if (unlikely(ret < ret_ok)) {
			PRINT_ERROR("cherokee_thread_new() failed %d\n", ret);
			return ret;
		}
		
		thread->thread_pref = (i % 2) ? thread_normal_tls : thread_tls_normal;

		cherokee_list_add (LIST(thread), &srv->thread_list);
	}
#endif

	/* Set keepalive limit for open connections.
	 * NOTE: this limit has to be lower (93%)
	 *       than conns_accept limit (95% - 99%) used for threads.
	 */
	srv->conns_keepalive_max = srv->conns_max - (srv->conns_max / 16);
	if (srv->conns_keepalive_max + 6 > srv->conns_max)
		srv->conns_keepalive_max = srv->conns_max - 6;

	/* OK, return.
	 */
	return ret_ok;
}


static ret_t
check_vservers_tls (cherokee_server_t *srv)
{
	ret_t            ret;
	cherokee_list_t *i;

	list_for_each (i, &srv->vservers) {
		ret = cherokee_virtual_server_has_tls (VSERVER(i));
		if (ret == ret_ok) {
			TRACE (ENTRIES, "Virtual Server %s: TLS enabled\n", VSERVER(i)->name.buf);
			return ret_ok;
		}
	}

	ret = cherokee_virtual_server_has_tls (srv->vserver_default);
	if (ret == ret_ok) {
		TRACE (ENTRIES, "Virtual Server %s: TLS enabled\n", "default");
		return ret_ok;
	}

	return ret_not_found;
}


static ret_t
init_vservers_tls (cherokee_server_t *srv)
{
#ifdef HAVE_TLS
	ret_t            ret;
	cherokee_list_t *i;

	/* Initialize the server TLS socket
	 */
	if (! cherokee_socket_is_connected (&srv->socket_tls)) {
		ret = initialize_server_socket (srv, &srv->socket_tls, srv->port_tls);
		if (unlikely(ret != ret_ok)) return ret;
	}

	/* Initialize TLS in all the virtual servers
	 */
	ret = cherokee_virtual_server_init_tls (srv->vserver_default);
	if (ret < ret_ok) {
		PRINT_ERROR_S ("Can not init TLS for the default virtual server\n");
		return ret_error;
	}

	list_for_each (i, &srv->vservers) {
		cherokee_virtual_server_t *vserver = VSERVER(i);

		ret = cherokee_virtual_server_init_tls (vserver);
		if (ret < ret_ok) {
			PRINT_ERROR ("Can not initialize TLS for `%s' virtual host\n", 
				     cherokee_buffer_is_empty(&vserver->name) ? "unknown" : vserver->name.buf);
		}
	}
#endif

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
		PRINT_ERROR ("WARNING: Unable to raise file descriptor limit to %d\n",
			     new_limit);
	}

	/* Update the new value
	 */
	ret = cherokee_sys_fdlimit_get (&cherokee_fdlimit);
	if (ret < ret_ok) {
		PRINT_ERROR_S ("ERROR: Unable to get file descriptor limit\n");
		return ret;
	}
	
	return ret_ok;
}


static ret_t
init_server_strings (cherokee_server_t *srv)
{
	ret_t ret;

	cherokee_buffer_clean (&srv->server_string);
	cherokee_buffer_clean (&srv->server_string_ext);
	cherokee_buffer_clean (&srv->server_string_w_port);
	cherokee_buffer_clean (&srv->server_string_w_port_tls);

	ret = cherokee_version_add_w_port (&srv->server_string_w_port, srv->server_token, srv->port);
	if (ret != ret_ok) return ret;

	ret = cherokee_version_add_w_port (&srv->server_string_w_port_tls, srv->server_token, srv->port_tls);
	if (ret != ret_ok) return ret;

	ret = cherokee_version_add (&srv->server_string_ext, srv->server_token);
	if (ret != ret_ok) return ret;

	ret = cherokee_version_add_simple (&srv->server_string, srv->server_token);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


ret_t
cherokee_server_initialize (cherokee_server_t *srv) 
{   
	int            re;
	ret_t          ret;
	struct passwd *ent;

	/* Build the server string
	 */
	ret = init_server_strings (srv);
	if (ret != ret_ok)
		return ret;

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
		PRINT_ERROR("Number of system fds too low: %d < %d\n", 
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
		PRINT_ERROR("Number of max. fds too low: %d < %d\n", 
			    srv->fdlimit_available, FD_NUM_MIN_AVAILABLE);
		return ret_error;
	}

	/* If the server has a previous server socket opened, Eg:
	 * because a SIGHUP, it shouldn't init the server socket.
	 */
	if (! cherokee_socket_is_connected (&srv->socket)) {
		ret = initialize_server_socket (srv, &srv->socket, srv->port);
		if (unlikely(ret != ret_ok)) return ret;
	}

	/* Init the SSL/TLS support
	 */
	srv->tls_enabled = (check_vservers_tls (srv) == ret_ok);

	if (srv->tls_enabled) {
		ret = init_vservers_tls (srv);
		if (ret != ret_ok) return ret;
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
	ent = getpwuid (srv->user);
	if (ent == NULL) {
		PRINT_ERROR ("Can't get username for UID %d\n", srv->user);
		return ret_error;
	}

	/* Chroot
	 */
	if (! cherokee_buffer_is_empty (&srv->chroot)) {
		re = chroot (srv->chroot.buf);
		srv->chrooted = (re == 0);
		if (srv->chrooted == 0) {
			PRINT_ERRNO (errno, "Cannot chroot() to '%s': '${errno}'", srv->chroot.buf);
			return ret_error;
		}
	} 

	/* Change the user
	 */
	ret = change_execution_user (srv, ent);
	if (ret != ret_ok) {
		return ret;
	}

	/* Change current directory
	 */
	re = chdir ("/");
	if (re < 0) {
		PRINT_ERRNO_S (errno, "Couldn't chdir(\"/\"): '${errno}'");
		return ret_error;
	}

	/* Create the threads
	 */
	ret = initialize_server_threads (srv);
	if (unlikely(ret < ret_ok))
		return ret;

	/* Print the server banner
	 */
	return print_banner (srv);
}


static void
flush_logs (cherokee_server_t *srv)
{
	cherokee_list_t   *i;
	cherokee_logger_t *logger;

	list_for_each (i, &srv->vservers) {
		logger = VSERVER_LOGGER(i);

		if (logger)
			cherokee_logger_flush (VSERVER_LOGGER(i));
	}

	logger = VSERVER_LOGGER(srv->vserver_default);
	if (logger) 
		cherokee_logger_flush (VSERVER_LOGGER(srv->vserver_default));
}


ret_t 
cherokee_server_stop (cherokee_server_t *srv)
{
	if (srv == NULL)
		return ret_ok;

	/* Close all connections
	 */
	close_all_connections (srv);

	/* Flush logs
	 */
	flush_logs (srv);

	return ret_ok;
}


ret_t 
cherokee_server_reinit (cherokee_server_t *srv)
{
	ret_t                        ret;
	cherokee_server_t           *new_srv   = NULL;
	cherokee_server_reinit_cb_t  reinit_cb = NULL; 

	if (srv->chrooted) {
		PRINT_ERROR_S ("WARNING: Chrooted cherokee cannot be reloaded. "
			       "Please, stop and restart it again.\n");
		return ret_ok;
	}

#if 0
	{
		int tmp;
		printf ("Handling server reinit. Press any key to continue..\n");
		read (0, &tmp, 1);
	}
#endif

	reinit_cb = srv->reinit_callback;

	/* Stop the server.
	 */
	ret = cherokee_server_stop (srv);
	if (ret != ret_ok)
		return ret;

	/* Destroy the server.
	 */
	ret = cherokee_server_free (srv);
	if (ret != ret_ok)
		return ret;
	srv = NULL;

	/* Create a new one
	 */
	ret = cherokee_server_new (&new_srv);
	if (ret != ret_ok)
		return ret;

	/* Send event
	 */
	if ((reinit_cb != NULL) && (new_srv != NULL)) {
		reinit_cb (new_srv);
	}

	return ret_ok;
}


static void
update_bogo_conns_num (cherokee_server_t *srv)
{
	cherokee_server_get_conns_num (srv, &srv->conns_num_bogo);
}


static void
update_bogo_now (cherokee_server_t *srv)
{
	time_t  newtime;

	/* Read the time
	 */
	newtime = time (NULL);
	if (srv->bogo_now >= newtime) 
		return;

	/* Update internal variables
	 */
	CHEROKEE_RWLOCK_WRITER (&srv->bogo_now_mutex);      /* 1.- lock as writer */

	srv->bogo_now = newtime;

	/* Convert time to both GMT and local time struct
	 */
	cherokee_gmtime    (&newtime, &srv->bogo_now_tmgmt);
	cherokee_localtime (&newtime, &srv->bogo_now_tmloc);

#ifdef HAVE_STRUCT_TM_GMTOFF
	srv->bogo_now_tzloc_sign = srv->bogo_now_tmloc.tm_gmtoff < 0 ? '-' : '+';
	srv->bogo_now_tzloc_offset = abs(srv->bogo_now_tmloc.tm_gmtoff / 3600);
#else
	srv->bogo_now_tzloc_sign = timezone < 0 ? '-' : '+';
	srv->bogo_now_tzloc_offset = abs(timezone / 3600);
#endif
	
	cherokee_buffer_clean (&srv->bogo_now_strgmt);
	{
	size_t  szlen = 0;
	char bufstr[DTM_SIZE_GMTTM_STR + 2];

	szlen = cherokee_dtm_gmttm2str (bufstr, sizeof(bufstr),
	                                &srv->bogo_now_tmgmt);
	cherokee_buffer_add (&srv->bogo_now_strgmt, bufstr, szlen);
	}

	/* NOTE: a local time string should have {+-}timezone_offset (hours)
	 *       added to date-time GMT string.
	 */

	CHEROKEE_RWLOCK_UNLOCK (&srv->bogo_now_mutex);      /* 2.- release */
}


ret_t
cherokee_server_unlock_threads (cherokee_server_t *srv)
{
	ret_t            ret;
	cherokee_list_t *i;

	/* Update bogo_now before launch the threads
	 */
	update_bogo_now (srv);

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

	/* Get the server time.
	 */
	update_bogo_now (srv);
	update_bogo_conns_num (srv);

	/* Execute thread step.
	 */
#ifndef HAVE_PTHREAD
	ret = cherokee_thread_step_SINGLE_THREAD (srv->main_thread);
#else
	if (srv->thread_num == 1)
		ret = cherokee_thread_step_SINGLE_THREAD (srv->main_thread);
	else
		ret = cherokee_thread_step_MULTI_THREAD (srv->main_thread, true);
#endif
	/* Logger flush 
	 */
	if (srv->log_flush_next < srv->bogo_now) {
		flush_logs (srv);
		srv->log_flush_next = srv->bogo_now + srv->log_flush_elapse;
	}

	/* Clean IO cache
	 */
	if (srv->iocache_clean_next < srv->bogo_now) {
		cherokee_iocache_clean_up (srv->iocache);	
		srv->iocache_clean_next = srv->bogo_now + IOCACHE_DEFAULT_CLEAN_ELAPSE;
	}

#ifdef _WIN32
	if (unlikely (cherokee_win32_shutdown_signaled(srv->bogo_now)))
		srv->wanna_exit = true;
#endif

	/* Wanna reinit or exit ?
	 */
	if (likely ((srv->wanna_reinit | srv->wanna_exit) == 0))
		return ret_eagain;

	/* Wanna reinit ?
	 */
	if (srv->wanna_reinit) {
		ret = cherokee_server_reinit (srv);

		/* NOTE: we MUST return in any case because
		 * the old srv server should have just been destroyed and
		 * a new server should have been created,
		 * in this case we cannot use the old srv ptr.
		 */
		return ret;
	}
	
	/* Wanna exit ?
	 */
	if (srv->wanna_exit) {
		flush_logs (srv);
		return ret_ok;
	}

	/* Should not be reached.
	 */
	return ret_eagain;
}


static ret_t 
add_encoder (cherokee_config_node_t *node, void *data)
{
	ret_t                           ret;
	cherokee_encoder_table_entry_t *enc      = NULL;
	cherokee_matching_list_t       *matching = NULL;
	cherokee_plugin_info_t         *info     = NULL;
 	cherokee_server_t              *srv      = SRV(data);

	TRACE(ENTRIES, "Encoder: %s\n", node->key.buf);

	/* Set the matching list
	 */
	ret = cherokee_matching_list_new (&matching);
	if (ret != ret_ok) goto error;

	ret = cherokee_matching_list_configure (matching, node);
	if (ret != ret_ok) goto error;	

	/* Load the module library and set the info
	 */
	ret = cherokee_plugin_loader_get (&srv->loader, node->key.buf, &info);
	if (ret != ret_ok) goto error;

	ret = cherokee_encoder_table_entry_new (&enc);
	if (ret != ret_ok) goto error;

	ret = cherokee_encoder_table_entry_get_info (enc, info);
	if (ret != ret_ok) goto error;

	cherokee_encoder_entry_set_matching_list (enc, matching);
 
	/* Set in the encoders table
	 */
	ret = cherokee_encoder_table_set (&srv->encoders, &node->key, enc);
	if (ret != ret_ok) goto error;

	return ret_ok;

error:
	TRACE(ENTRIES, "Could not add '%s' encoder\n", node->key.buf);
	return ret;
}


static ret_t 
add_vserver (cherokee_config_node_t *conf, void *data)
{
	ret_t                      ret;
	cherokee_virtual_server_t *vsrv;
 	cherokee_server_t         *srv = SRV(data);

	TRACE (ENTRIES, "Adding vserver %s\n", conf->key.buf);

	if (equal_buf_str (&conf->key, "default")) {
		vsrv = srv->vserver_default;

	} else {
		/* Create a new vserver and enqueue it
		 */
		ret = cherokee_virtual_server_new (&vsrv, srv);
		if (ret != ret_ok) return ret;

		cherokee_list_add (LIST(vsrv), &srv->vservers);
	}

	ret = cherokee_virtual_server_configure (vsrv, &conf->key, conf);
	if (ret != ret_ok) return ret;	

	return ret_ok;
}


static ret_t 
configure_server_property (cherokee_config_node_t *conf, void *data)
{
	ret_t              ret;
	char              *key = conf->key.buf;
	cherokee_server_t *srv = SRV(data);

	if (equal_buf_str (&conf->key, "port")) {
		srv->port = atoi(conf->val.buf);

	} else if (equal_buf_str (&conf->key, "port_tls")) {
		srv->port_tls = atoi(conf->val.buf);

	} else if (equal_buf_str (&conf->key, "fdlimit")) {
		srv->fdlimit_custom = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "listen_queue")) {
		srv->listen_queue = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "thread_number")) {
		srv->thread_num = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "sendfile_min")) {
		srv->sendfile.min = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "sendfile_max")) {
		srv->sendfile.max = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "max_connection_reuse")) {
		srv->conns_reuse_max = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "ipv6")) {
		srv->ipv6 = !!atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "timeout")) {
		srv->timeout = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "log_flush_elapse")) {
		srv->log_flush_elapse = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "keepalive")) {
		srv->keepalive = !!atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "keepalive_max_requests")) {
		srv->keepalive_max = atoi (conf->val.buf);

	} else if (equal_buf_str (&conf->key, "unix_socket")) {
		cherokee_buffer_clean (&srv->unix_socket);
		cherokee_buffer_add_buffer (&srv->unix_socket, &conf->val);

	} else if (equal_buf_str (&conf->key, "panic_action")) {
		cherokee_buffer_clean (&srv->panic_action);
		cherokee_buffer_add_buffer (&srv->panic_action, &conf->val);

	} else if (equal_buf_str (&conf->key, "chroot")) {
		cherokee_buffer_clean (&srv->chroot);
		cherokee_buffer_add_buffer (&srv->chroot, &conf->val);

	} else if (equal_buf_str (&conf->key, "pid_file")) {
		cherokee_buffer_clean (&srv->pidfile);
		cherokee_buffer_add_buffer (&srv->pidfile, &conf->val);

	} else if (equal_buf_str (&conf->key, "listen")) {
		cherokee_buffer_clean (&srv->listen_to);
		cherokee_buffer_add_buffer (&srv->listen_to, &conf->val);

	} else if (equal_buf_str (&conf->key, "poll_method")) {
		char    *str          = conf->val.buf;
		cuint_t  sys_fd_limit = 0;
		cuint_t  thr_fd_limit = 0;

		/* Convert poll method string to type.
		 */
		ret = cherokee_fdpoll_str_to_method(str, &(srv->fdpoll_method));
		if (ret != ret_ok) {
			PRINT_MSG ("ERROR: Unknown polling method '%s'\n", str);
			return ret_error;
		}

		/* Verify whether the method is supported by this OS.
		 */
		ret = cherokee_fdpoll_get_fdlimits (srv->fdpoll_method, &sys_fd_limit, &thr_fd_limit);
		switch (ret) {
		case ret_ok:
			break;
		case ret_no_sys:
			PRINT_MSG ("ERROR: polling method '%s' is NOT supported by this OS\n", str);
			return ret;
		default:
			PRINT_MSG ("ERROR: polling method '%s' has NOT been recognized (internal ERROR)\n", str);
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
			PRINT_MSG ("ERROR: Unknown server token '%s'\n", conf->val.buf);
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
			PRINT_MSG ("ERROR: Unknown thread policy '%s'\n", conf->val.buf);
			return ret_error;
		}
#else
		PRINT_MSG ("WARNING: Ignoring thread_policy entry '%s'\n", conf->val.buf);
#endif

	} else if (equal_buf_str (&conf->key, "user")) {
		struct passwd *pwd;
	   
		pwd = (struct passwd *) getpwnam (conf->val.buf);
		if (pwd == NULL) {
			 PRINT_MSG ("ERROR: User '%s' not found in the system\n", conf->val.buf);
			 return ret_error;
		}
		srv->user = pwd->pw_uid;		

	} else if (equal_buf_str (&conf->key, "group")) {
		struct group *grp;
		
		grp = (struct group *) getgrnam (conf->val.buf);
		if (grp == NULL) {
			PRINT_MSG ("ERROR: Group '%s' not found in the system\n", conf->val.buf);
			return ret_error;
		}		
		srv->group = grp->gr_gid;

	} else if (equal_buf_str (&conf->key, "encoder")) {
		ret = cherokee_config_node_while (conf, add_encoder, srv);
		if (ret != ret_ok) return ret;

	} else if (equal_buf_str (&conf->key, "module_dir") ||
		   equal_buf_str (&conf->key, "module_deps")) {
		/* Ignore it: Previously handled 
		 */

	} else {
		PRINT_MSG ("ERROR: Server parser: Unknown key \"%s\"\n", key);
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

		/* Rest of the properties
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

	/* Load the virtual servers
	 */
	TRACE (ENTRIES, "Configuring %s\n", "virtual servers");
	ret = cherokee_config_node_get (&srv->config, "vserver", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_config_node_while (subconf, add_vserver, srv);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}


static ret_t
perform_post_configure_checks (cherokee_server_t *srv)
{
	int              re;
	cherokee_list_t *i;

	/* Ensure that Virtual servers have a document root
	 */
	re = cherokee_buffer_is_empty (&(srv->vserver_default)->root);
	if (re) {
		PRINT_MSG_S ("Default Virtual Server needs a Document Root\n");
		return ret_error;
	}

	list_for_each (i, &srv->vservers) {
		re = cherokee_buffer_is_empty (&VSERVER(i)->root);
		if (re) {
			PRINT_MSG_S ("Virtual Server  needs a Document Root\n");
			return ret_error;
		}
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

	/* Ensure that the server is ready
	 */
	ret = perform_post_configure_checks (srv);
	if (ret != ret_ok) return ret;

	/* Clean up
	 */
	ret = cherokee_config_node_mrproper (&srv->config);
	if (ret != ret_ok) return ret;
	
	return ret_ok;
}


ret_t 
cherokee_server_read_config_file (cherokee_server_t *srv, char *fullpath)
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

	/* Ensure that the server is ready
	 */
	ret = perform_post_configure_checks (srv);
	if (ret != ret_ok) return ret;

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
#ifndef _WIN32
        pid_t child_pid;

	TRACE (ENTRIES, "server (%p) about to become evil", srv);

	child_pid = fork();
        switch (child_pid) {
	case -1:
                PRINT_ERROR_S ("Could not fork\n");
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
#endif

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
cherokee_server_get_total_traffic (cherokee_server_t *srv, size_t *rx, size_t *tx)
{
	cherokee_list_t *i;

	*rx = srv->vserver_default->data.rx;
	*tx = srv->vserver_default->data.tx;

	list_for_each (i, &srv->vservers) {
		*rx += VSERVER(i)->data.rx;
		*tx += VSERVER(i)->data.tx;
	}

	return ret_ok;
}


ret_t 
cherokee_server_handle_HUP (cherokee_server_t *srv, cherokee_server_reinit_cb_t callback)
{
	srv->reinit_callback = callback;
	srv->wanna_reinit    = true;

	return ret_ok;
}


ret_t 
cherokee_server_handle_TERM (cherokee_server_t *srv)
{
	if (srv != NULL)
		srv->wanna_exit = true;

	return ret_ok;
}


ret_t
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
		PRINT_ERROR ("PANIC: re-panic: '%s', status %d\n", cmd.buf, val);
	}

	cherokee_buffer_mrproper (&cmd);
fin:
	abort();
}


ret_t 
cherokee_server_del_connection (cherokee_server_t *srv, char *id_str)
{
	culong_t         id;
	cherokee_list_t *t, *c;
	
	id = strtol (id_str, NULL, 10);

	list_for_each (t, &srv->thread_list) {
		cherokee_thread_t *thread = THREAD(t);

		CHEROKEE_MUTEX_LOCK (&thread->ownership);

		list_for_each (c, &THREAD(t)->active_list) {
			cherokee_connection_t *conn = CONN(c);

			if (conn->id == id) {
				if ((conn->phase != phase_nothing) && 
				    (conn->phase != phase_lingering))
				{
					conn->phase = phase_shutdown;
					CHEROKEE_MUTEX_UNLOCK (&thread->ownership);
					return ret_ok;
				}
			}
		}
		CHEROKEE_MUTEX_UNLOCK (&thread->ownership);
	}

	return ret_not_found;
}


ret_t 
cherokee_server_log_reopen (cherokee_server_t *srv)
{
	ret_t            ret;
	cherokee_list_t *i;

	if (srv->vserver_default->logger) {
		ret = cherokee_logger_reopen (srv->vserver_default->logger);
		if (unlikely (ret != ret_ok)) return ret;		
	}

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

	if (srv->vserver_default->logger) {
		ret = cherokee_logger_set_backup_mode (srv->vserver_default->logger, active);
		if (unlikely (ret != ret_ok)) return ret;
	}

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

	if (srv->vserver_default->logger) {
		cherokee_logger_get_backup_mode (srv->vserver_default->logger, active);
		if (*active == true) return ret_ok;
	}

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
cherokee_server_write_pidfile (cherokee_server_t *srv)
{
	size_t  written;
	FILE   *file;
	CHEROKEE_TEMP(buffer, 10);

	if (cherokee_buffer_is_empty (&srv->pidfile))
		return ret_not_found;

	file = fopen (srv->pidfile.buf, "w+");
	if (file == NULL) {
		PRINT_ERRNO (errno, "Cannot write PID file '%s': '${errno}'", srv->pidfile.buf);
		return ret_error;
	}

	snprintf (buffer, buffer_size, "%d\n", getpid());
	written = fwrite (buffer, 1, strlen(buffer), file);
	fclose (file);

	if (written <= 0)
		return ret_error;

	return ret_ok;
}


ret_t 
cherokee_server_get_vserver (cherokee_server_t *srv, cherokee_buffer_t *name, cherokee_virtual_server_t **vsrv)
{
	cint_t                     re;
	ret_t                      ret;
	cherokee_list_t           *i;
	cherokee_virtual_server_t *vserver;

	list_for_each (i, &srv->vservers) {
		vserver = VSERVER(i);

		ret = cherokee_vserver_names_find (&vserver->domains, name);
		if (ret == ret_ok) {
			TRACE (ENTRIES, "Virtual server '%s' matched domain '%s'\n", vserver->name.buf, name->buf);
			*vsrv = vserver;
			return ret_ok;
		}

		re = cherokee_buffer_cmp_buf (name, &vserver->name);
		if (re == 0) {
			TRACE (ENTRIES, "Virtual server '%s' matched by its alias\n", vserver->name.buf);
			*vsrv = vserver;
			return ret_ok;
		}
	}

	*vsrv = srv->vserver_default;
	return ret_ok;
}
