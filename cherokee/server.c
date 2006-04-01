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
#include "ncpus.h"
#include "mime.h"
#include "list_ext.h"
#include "util.h"
#include "fdpoll.h"
#include "fdpoll-protected.h"
#include "regex.h"
#include "fcgi_manager.h"
#include "connection-protected.h"
#include "nonce.h"

#define ENTRIES "core,server"


ret_t
cherokee_server_new  (cherokee_server_t **srv)
{
	ret_t ret;

	/* Get memory
	 */
	CHEROKEE_NEW_STRUCT(n, server);
	
	/* Thread list
	 */
	INIT_LIST_HEAD(&(n->thread_list));

	/* Sockets
	 */
	n->socket          = -1;
	n->socket_tls      = -1;
	n->ipv6            =  1;
	n->fdpoll_method   = cherokee_poll_UNSET;

	/* Config files
	 */
	n->config_file     = NULL;	
	n->icons_file      = NULL;
	
	/* Mime types
	 */
	n->mime_file       = NULL;
	n->mime            = NULL;

	/* Exit related
	 */
	n->wanna_exit      = false;
	n->panic_action    = NULL;
	n->reinit_callback = NULL;
	
	/* Server config
	 */
	n->port            = 80;
	n->port_tls        = 443;
	n->tls_enabled     = false;

	n->listen_to       = NULL;
	n->fdwatch_msecs   = 999;

	n->start_time      = time(NULL);

	n->keepalive       = true;
	n->keepalive_max   = MAX_KEEPALIVE;

	n->ncpus           = -1;
	n->thread_num      = -1;
	n->thread_policy   = -1;

	n->chroot          = NULL;
	n->chrooted        = 0;

	n->user_orig       = getuid();
	n->user            = n->user_orig;
	n->group_orig      = getgid();
	n->group           = n->group_orig;

	n->timeout         = 15;

	n->max_fds         = -1;
	n->system_fd_limit = -1;
	n->max_conn_reuse  = -1;

	n->listen_queue    = 1024;
	n->sendfile.min    = 32768;
	n->sendfile.max    = 2147483647;

	n->regexs          = NULL;
	n->icons           = NULL;
	n->iocache         = NULL;

	/* Bogo now
	 */
	n->bogo_now        = 0;
	CHEROKEE_RWLOCK_INIT (&n->bogo_now_mutex, NULL);

	CHEROKEE_RWLOCK_WRITER (&n->bogo_now_mutex);
	cherokee_buffer_new (&n->bogo_now_string);
	cherokee_buffer_ensure_size (n->bogo_now_string, 100);	
	CHEROKEE_RWLOCK_UNLOCK (&n->bogo_now_mutex);

	/* Time managing hack
	 */
	cherokee_buffer_new (&n->timeout_header);
	cherokee_buffer_add (n->timeout_header, "Keep-Alive: timeout=15"CRLF, 24);

	/* Accepting mutexes
	 */
#ifdef HAVE_TLS
	CHEROKEE_MUTEX_INIT (&n->accept_tls_mutex, NULL);
#endif
	CHEROKEE_MUTEX_INIT (&n->accept_mutex, NULL);

#ifndef CHEROKEE_EMBEDDED
	/* Icons 
	 */
	cherokee_icons_new (&n->icons);
	return_if_fail (n->icons != NULL, ret_nomem);

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
#endif

	/* Module loader
	 */
	cherokee_module_loader_init (&n->loader);

	/* Virtual servers table
	 */
	INIT_LIST_HEAD (&n->vservers);

	cherokee_table_new (&n->vservers_ref);
	return_if_fail (n->vservers_ref!=NULL, ret_nomem);

	cherokee_virtual_server_new (&n->vserver_default);
	return_if_fail (n->vserver_default!=NULL, ret_nomem);
		
	/* Encoders 
	 */
	cherokee_encoder_table_new (&n->encoders);
	return_if_fail (n->encoders != NULL, ret_nomem);

	/* Server string
	 */
	n->server_token = cherokee_version_full;
	cherokee_buffer_new (&n->server_string);

	/* Loggers
	 */
	n->log_flush_next   = 0;
	n->log_flush_elapse = 10;

	cherokee_logger_table_new (&n->loggers);
	return_if_fail (n->loggers != NULL, ret_nomem);

	/* TLS
	 */
	ret = cherokee_tls_init();
	if (ret < ret_ok) {
		return ret;
	}

	/* PID
	 */
	cherokee_buffer_init (&n->pidfile);
	
	/* Return the object
	 */
	*srv = n;
	return ret_ok;
}


static void
close_all_connections (cherokee_server_t *srv)
{
	list_t *i;

	cherokee_thread_close_all_connections (srv->main_thread);
	
	list_for_each (i, &srv->thread_list) {
		cherokee_thread_close_all_connections (THREAD(i));
	}
}


static void
free_virtual_servers (cherokee_server_t *srv)
{
	list_t *i, *j;

	list_for_each_safe (i, j, &srv->vservers) {
		cherokee_virtual_server_free (VSERVER(i));
		list_del(i);
	}
}


ret_t
cherokee_server_free (cherokee_server_t *srv)
{
	close (srv->socket);
	
	if (srv->socket_tls != -1) {
		close (srv->socket_tls);		
	}

	cherokee_encoder_table_free (srv->encoders);
	cherokee_logger_table_free (srv->loggers);

#ifdef HAVE_TLS
	CHEROKEE_MUTEX_DESTROY (&srv->accept_tls_mutex);
#endif
	CHEROKEE_MUTEX_DESTROY (&srv->accept_mutex);

#ifndef CHEROKEE_EMBEDDED
	/* Nonces
	 */
	cherokee_nonce_table_free (srv->nonces);
	srv->nonces = NULL;

	/* Icons 
	 */
	cherokee_icons_free (srv->icons);
	if (srv->icons_file != NULL) {
		free (srv->icons_file);
		srv->icons_file = NULL;
	}

	/* Mime
	 */
	if (srv->mime_file != NULL) {
		free (srv->mime_file);
		srv->mime_file = NULL;
	}

	if (srv->mime != NULL) {
		cherokee_mime_free (srv->mime);
		srv->mime = NULL;
	}

	cherokee_iocache_free_default (srv->iocache);

	cherokee_regex_table_free (srv->regexs);
#endif
	
	/* Virtual servers
	 */
	cherokee_virtual_server_free (srv->vserver_default);
	srv->vserver_default = NULL;

	free_virtual_servers (srv);
	cherokee_table_free (srv->vservers_ref);

	cherokee_buffer_free (srv->bogo_now_string);
	cherokee_buffer_free (srv->server_string);

	cherokee_buffer_free (srv->timeout_header);

	if (srv->listen_to != NULL) {
		free (srv->listen_to);
		srv->listen_to = NULL;
	}

	if (srv->chroot != NULL) {
		free (srv->chroot);
		srv->chroot = NULL;
	}

	/* Clean config files entries
	 */
	if (srv->config_file != NULL) {
		free (srv->config_file);
		srv->config_file = NULL;
	}

	if (srv->panic_action != NULL) {
		free (srv->panic_action);
		srv->panic_action = NULL;
	}

	cherokee_buffer_mrproper (&srv->pidfile);

	/* Module loader:
	 * It must be the last action to perform because
	 * it will close all the opened modules
	 */
	cherokee_module_loader_mrproper (&srv->loader);

	free (srv);	
	return ret_ok;
}


static ret_t
change_execution_user (cherokee_server_t *srv, struct passwd *ent)
{
	int error;

/*         /\* Get user information */
/* 	 *\/ */
/* 	ent = getpwuid (srv->user); */
/* 	if (ent == NULL) { */
/* 		PRINT_ERROR ("Can't get username for UID %d\n", srv->user); */
/* 		return ret_error; */
/* 	} */
	
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
set_server_socket_opts (int socket)
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
	re = setsockopt (socket, SOL_SOCKET, TCP_MAXSEG, &on, sizeof(on));
	if (re != 0) return ret_error;
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
	 * Maybe add support for the FreeBSD accept filters:
	 * http://www.freebsd.org/cgi/man.cgi?query=accf_http
	 */

	return ret_ok;
}


static ret_t
initialize_server_socket4 (cherokee_server_t *srv, unsigned short port, int *srv_socket_ret)
{
	int re;
	int srv_socket;
	struct sockaddr_in sockaddr;

	srv_socket = socket (AF_INET, SOCK_STREAM, 0);
	if (srv_socket < 0) {
		PRINT_ERROR_S ("Error creating IPv4 server socket\n");
		exit(EXIT_CANT_CREATE_SERVER_SOCKET4);
	}

	*srv_socket_ret = srv_socket;
	set_server_socket_opts(srv_socket);

	/* Bind the socket
	 */
	memset (&sockaddr, 0, sizeof(struct sockaddr_in));

	sockaddr.sin_port   = htons (port);
	sockaddr.sin_family = AF_INET;

	if (srv->listen_to == NULL) {
		sockaddr.sin_addr.s_addr = INADDR_ANY; 
	} else {
#ifdef HAVE_INET_PTON
		inet_pton (sockaddr.sin_family, srv->listen_to, &sockaddr.sin_addr);
#else
		/* IPv6 needs inet_pton. inet_addr just doesn't work without
		 * it. We'll suppose then that we haven't IPv6 support 
		 */
		sockaddr.sin_addr.s_addr = inet_addr (srv->listen_to);
#endif
	}

	re = bind (srv_socket, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in));
	return (re == 0) ? ret_ok : ret_error;
}


static ret_t
initialize_server_socket6 (cherokee_server_t *srv, unsigned short port, int *srv_socket_ret)
{
#ifdef HAVE_IPV6
	int re;
	int srv_socket;
	struct sockaddr_in6 sockaddr;

	/* Create the socket
	 */
	srv_socket = socket (AF_INET6, SOCK_STREAM, 0);
	if (srv_socket < 0) {
		PRINT_ERROR_S ("Error creating IPv6 server socket.. switching to IPv4\n");
		srv->ipv6 = 0;
		return ret_error;
	}

	*srv_socket_ret = srv_socket;
	set_server_socket_opts(srv_socket);

	/* Bind the socket
	 */
	memset (&sockaddr, 0, sizeof(struct sockaddr_in6));

	sockaddr.sin6_port   = htons (port);  /* Transport layer port #   */
	sockaddr.sin6_family = AF_INET6;      /* AF_INET6                 */

	if (srv->listen_to == NULL) {
		sockaddr.sin6_addr = in6addr_any; 
	} else {
		inet_pton (sockaddr.sin6_family, srv->listen_to, &sockaddr.sin6_addr);
	}

	re = bind (srv_socket, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in6));
	return (re == 0) ? ret_ok : ret_error;
#endif
}


static ret_t
print_banner (cherokee_server_t *srv)
{
	char *p;
	char *method;
	CHEROKEE_NEW(n,buffer);

	/* First line
	 */
	cherokee_buffer_add_va (n, "Cherokee Web Server %s: ", PACKAGE_VERSION);

	if (srv->socket_tls != -1 && srv->socket != -1) {
		cherokee_buffer_add_va (n, "Listening on ports %d and %d", srv->port, srv->port_tls);
	} else {
		cherokee_buffer_add_va (n, "Listening on port %d", 
					(srv->socket != -1)? srv->port : srv->port_tls);
	}

	if (srv->chrooted) {
		cherokee_buffer_add (n, ", chrooted", 10);
	}

	/* TLS / SSL
	 */
	if (srv->tls_enabled) {
#if defined(HAVE_GNUTLS)
		cherokee_buffer_add (n, ", with TLS support via GNUTLS", 29);
#elif defined(HAVE_OPENSSL)
		cherokee_buffer_add (n, ", with TLS support via OpenSSL", 30);
#endif
	} else {
		cherokee_buffer_add (n, ", TLS disabled", 14);		
	}

	/* IPv6
	 */
#ifdef HAVE_IPV6
	if (srv->ipv6) {
		cherokee_buffer_add (n, ", IPv6 enabled", 14);		
	} else
#endif
		cherokee_buffer_add (n, ", IPv6 disabled", 14);

	/* Polling method
	 */
	cherokee_fdpoll_get_method_str (srv->main_thread->fdpoll, &method);
	cherokee_buffer_add_va (n, ", using %s", method);

	/* File descriptor limit
	 */
	cherokee_buffer_add_va (n, ", %d fds limit", srv->system_fd_limit);

	/* Threading stuff
	 */
	if (srv->thread_num <= 1) {
		cherokee_buffer_add (n, ", single thread", 15);
	} else {
		cherokee_buffer_add_va (n, ", %d threads", srv->thread_num);
		cherokee_buffer_add_va (n, ", %d fds in each", srv->system_fd_limit / (srv->thread_num));	

		switch (srv->thread_policy) {
#ifdef HAVE_PTHREAD
		case SCHED_FIFO:
			cherokee_buffer_add (n, ", FIFO scheduling policy", 24);
			break;
		case SCHED_RR:
			cherokee_buffer_add (n, ", RR scheduling policy", 22);
			break;
#endif
		default:
			cherokee_buffer_add (n, ", standard scheduling policy", 28);
			break;
		}
	}

	/* Print it!
	 */
	for (p = n->buf+TERMINAL_WIDTH; p < n->buf+n->len; p+=75) {
		while (*p != ',') p--;
		*p = '\n';
	}

	printf ("%s\n", n->buf);
	cherokee_buffer_free (n);
	
	return ret_ok;
}


static ret_t
initialize_server_socket (cherokee_server_t *srv, unsigned short port, int *srv_socket_ptr)
{
	int   flags;
	int   srv_socket;
	ret_t ret = ret_error;

#ifdef HAVE_IPV6
	if (srv->ipv6) {
		ret = initialize_server_socket6 (srv, port, srv_socket_ptr);
	}
#endif

	if ((srv->ipv6 == 0) || (ret != ret_ok)) {
		ret = initialize_server_socket4 (srv, port, srv_socket_ptr);
	}

	if (ret != 0) {
		uid_t uid = getuid();
		gid_t gid = getgid();

		PRINT_ERROR ("Can't bind() socket (port=%d, UID=%d, GID=%d)\n", port, uid, gid);
		return ret_error;
	}
	   
	srv_socket = *srv_socket_ptr;

	/* Set no-delay mode
	 */
#ifdef _WIN32
	flags = 1;
	ret = ioctlsocket (srv_socket, FIONBIO, (u_long)&flags);
#else
	flags = fcntl (srv_socket, F_GETFL, 0);
	return_if_fail (flags != -1, ret_error);
	
	ret = fcntl (srv_socket, F_SETFL, flags | O_NDELAY);
	return_if_fail (ret >= 0, ret_error);
#endif	

	/* Listen
	 */
	ret = listen (srv_socket, srv->listen_queue);
	if (ret < 0) {
		close (srv_socket);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
initialize_server_threads (cherokee_server_t *srv)
{	
	ret_t ret;
	int   i, fds_per_thread;

	/* Get the system fd number limit
	 */
	fds_per_thread = srv->system_fd_limit / srv->thread_num;

	/* Create the main thread
	 */
	ret = cherokee_thread_new (&srv->main_thread, srv, thread_sync, 
				   srv->fdpoll_method, srv->system_fd_limit, fds_per_thread);
	if (unlikely(ret < ret_ok)) return ret;

	/* If Cherokee is compiled in single thread mode, it has to
	 * add the server socket to the fdpoll of the sync thread
	 */
#ifndef HAVE_PTHREAD
	ret = cherokee_fdpoll_add (srv->main_thread->fdpoll, srv->socket, 0);
	if (unlikely(ret < ret_ok)) return ret;

	if (srv->tls_enabled) {
		ret = cherokee_fdpoll_add (srv->main_thread->fdpoll, srv->socket_tls, 0);
		if (unlikely(ret < ret_ok)) return ret;
	}
#endif

	/* If Cherokee is compiles in multi-thread mode, it has to
	 * launch the threads
	 */
#ifdef HAVE_PTHREAD
	for (i=0; i<srv->thread_num - 1; i++) {
		cherokee_thread_t *thread;

		ret = cherokee_thread_new (&thread, srv, thread_async, 
					   srv->fdpoll_method, srv->system_fd_limit, fds_per_thread);
		if (unlikely(ret < ret_ok)) return ret;
		
		thread->thread_pref = (i % 2)? thread_normal_tls : thread_tls_normal;

		list_add ((list_t *)thread, &srv->thread_list);
	}
#endif

	return ret_ok;
}


static void
for_each_vserver_init_tls_func (const char *key, void *value)
{
	ret_t ret;
	cherokee_virtual_server_t *vserver = VSERVER(value);

	ret = cherokee_virtual_server_init_tls (vserver);
	if (ret < ret_ok) {
		PRINT_ERROR ("Can not initialize TLS for `%s' virtual host\n", 
			     cherokee_buffer_is_empty(vserver->name) ? "unknown" : vserver->name->buf);
	}
}


static int  
while_vserver_check_tls_func (const char *key, void *value, void *param)
{
	cherokee_boolean_t found;
	
	found = (cherokee_virtual_server_have_tls (VSERVER(value)) == ret_ok);

	return (found) ? 0 : 1;
}


static ret_t
init_vservers_tls (cherokee_server_t *srv)
{
#ifdef HAVE_TLS
	ret_t ret;

	/* Initialize the server TLS socket
	 */
	if (srv->socket_tls == -1) {
		ret = initialize_server_socket (srv, srv->port_tls, &srv->socket_tls);
		if (unlikely(ret != ret_ok)) return ret;
	}

	/* Initialize TLS in all the virtual servers
	 */
	ret = cherokee_virtual_server_init_tls (srv->vserver_default);
	if (ret < ret_ok) {
		PRINT_ERROR_S ("Can not init TLS for the default virtual server\n");
		return ret_error;
	}

	cherokee_table_foreach (srv->vservers_ref, for_each_vserver_init_tls_func);
#endif

	return ret_ok;	
}


static ret_t
set_fdmax_limit (cherokee_server_t *srv)
{
	ret_t ret;

	/* Try to raise the fd number system limit
	 */
	if (srv->max_fds != -1) {
		/* Set it	
		 */
		ret = cherokee_sys_fdlimit_set (srv->max_fds);
		if (ret < ret_ok) {
			PRINT_ERROR ("WARNING: Unable to set file descriptor limit to %d\n",
				     srv->max_fds);
		}
	}

	/* Get system fd limimt.. has it been increased?
	 */
	ret = cherokee_sys_fdlimit_get (&srv->system_fd_limit);
	if (ret < ret_ok) {
		PRINT_ERROR_S ("ERROR: Unable to get file descriptor limit\n");
		return ret;
	}

	return ret_ok;
}


static void
build_server_string (cherokee_server_t *srv)
{
	cherokee_buffer_clean (srv->server_string);

	/* Cherokee
	 */
	cherokee_buffer_add (srv->server_string, "Cherokee", 8); 
	if (srv->server_token <= cherokee_version_product) 
		return;

	/* Cherokee/x.y
	 */
	cherokee_buffer_add_va (srv->server_string, "/%s.%s", PACKAGE_MAJOR_VERSION, PACKAGE_MINOR_VERSION);
	if (srv->server_token <= cherokee_version_minor) 
		return;

	/* Cherokee/x.y.z-betaXX
	 */
	cherokee_buffer_add_va (srv->server_string, ".%s", PACKAGE_MICRO_VERSION PACKAGE_PATCH_VERSION);
	if (srv->server_token <= cherokee_version_minimal) 
		return;

	/* Cherokee/x.y.z-betaXX (UNIX)
	 */
	cherokee_buffer_add_va (srv->server_string, " (%s)", OS_TYPE);
	if (srv->server_token <= cherokee_version_os) return;

	/* Cherokee/x.y.z-betaXX (UNIX) Ext1/x.y Ext2/x.y
	 * TODO
	 */
}


ret_t
cherokee_server_init (cherokee_server_t *srv) 
{   
	ret_t          ret;
	struct passwd *ent;

	/* Build the server string
	 */
	build_server_string (srv);

	/* Set the FD number limit
	 */
	ret = set_fdmax_limit (srv);
	if (unlikely(ret < ret_ok)) return ret;

	/* If the server has a previous server socket opened, Eg:
	 * because a SIGHUP, it shouldn't init the server socket.
	 */
	if (srv->socket == -1) {
		ret = initialize_server_socket (srv, srv->port, &srv->socket);
		if (unlikely(ret != ret_ok)) return ret;
	}

	/* Look if TLS is enabled
	 */
	srv->tls_enabled = (cherokee_virtual_server_have_tls (srv->vserver_default) == ret_ok);

	if (srv->tls_enabled == false) {
		ret = cherokee_table_while (srv->vservers_ref, while_vserver_check_tls_func, NULL, NULL, NULL);
		srv->tls_enabled = (ret == ret_ok);
	}

	if (srv->tls_enabled) {
		ret = init_vservers_tls (srv);
		if (unlikely(ret != ret_ok)) return ret;
	}

        /* Get the CPU number
	 */
#ifndef CHEROKEE_EMBEDDED
	dcc_ncpus (&srv->ncpus);
#else
	srv->ncpus = 1;
#endif
	if (srv->ncpus == -1) {
		PRINT_ERROR_S ("Can not deternime the number of processors\n");
		srv->ncpus = 1;
	}

	/*  Maybe recalculate the thread number
	 */
	if (srv->thread_num == -1) {
#ifdef HAVE_PTHREAD
		srv->thread_num = srv->ncpus * 5;
#else
		srv->thread_num = 1;
#endif
	}
		
	/* Check the number of reusable connections
	 */
	if (srv->max_conn_reuse == -1) {
		srv->max_conn_reuse = DEFAULT_CONN_REUSE;
	}

	/* Get the passwd file entry before chroot
	 */
	ent = getpwuid (srv->user);
	if (ent == NULL) {
		PRINT_ERROR ("Can't get username for UID %d\n", srv->user);
		return ret_error;
	}

	/* Chroot
	 */
	if (srv->chroot) {
		srv->chrooted = (chroot (srv->chroot) == 0);
		if (srv->chrooted == 0) {
			PRINT_ERROR ("Cannot chroot() to '%s': %s\n", srv->chroot, strerror(errno));
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
	chdir ("/");

	/* Create the threads
	 */
	ret = initialize_server_threads (srv);
	if (unlikely(ret < ret_ok)) return ret;

	/* Print the server banner
	 */
	print_banner (srv);

	return ret_ok;
}


static void
flush_vserver (const char *key, void *value)
{
	/* There's no logger in this virtual server
	 */
	if ((value == NULL) || (VSERVER_LOGGER(value) == NULL))
		return;

	cherokee_logger_flush (VSERVER_LOGGER(value));
}


void static 
flush_logs (cherokee_server_t *srv)
{
	flush_vserver (NULL, srv->vserver_default);
	cherokee_table_foreach (srv->vservers_ref, flush_vserver);
}

static ret_t
destroy_all_threads (cherokee_server_t *srv)
{
	list_t *i, *tmp;

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

	/* Let's destroy the threads
	 */
	list_for_each_safe (i, tmp, &srv->thread_list) {
		cherokee_thread_t *thread = THREAD(i);

		cherokee_thread_wait_end (thread);
		cherokee_thread_free (thread);
	}

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

	/* Close all connections
	 */
	close_all_connections (srv);

	/* Destroy all the threads
	 */
	destroy_all_threads (srv);

	/* Destroy the server object
	 */
	ret = cherokee_server_free (srv);
	if (ret != ret_ok) return ret;
	srv = NULL;

	/* Create a new one
	 */
	ret = cherokee_server_new (&new_srv);
	if (ret != ret_ok) return ret;

	/* Send event
	 */
	if ((reinit_cb != NULL) && (new_srv != NULL)) {
		reinit_cb (new_srv);
	}

	return ret_ok;
}


static void
update_bogo_now (cherokee_server_t *srv)
{
	time_t       prev;
	static long *this_timezone = NULL;

	CHEROKEE_RWLOCK_WRITER (&srv->bogo_now_mutex);      /* 1.- lock as writer */

	prev = srv->bogo_now;
	srv->bogo_now = time (NULL);
	cherokee_localtime (&srv->bogo_now, &srv->bogo_now_tm);
	
	/* Update time string if needed
	 */
	if (prev < srv->bogo_now) {
		int z;

		cherokee_buffer_clean (srv->bogo_now_string);

		if (this_timezone == NULL) 
			this_timezone = cherokee_get_timezone_ref();
		z = - (*this_timezone / 60);

		cherokee_buffer_add_va (srv->bogo_now_string, "%s, %02d %s %d %02d:%02d:%02d GMT%c%d",
					cherokee_weekdays[srv->bogo_now_tm.tm_wday], 
					srv->bogo_now_tm.tm_mday,
					cherokee_months[srv->bogo_now_tm.tm_mon], 
					srv->bogo_now_tm.tm_year + 1900,
					srv->bogo_now_tm.tm_hour,
					srv->bogo_now_tm.tm_min,
					srv->bogo_now_tm.tm_sec,
					(z < 0) ? '-' : '+',
					(z / 60));
	}

	CHEROKEE_RWLOCK_UNLOCK (&srv->bogo_now_mutex);      /* 2.- release */
}


ret_t
cherokee_server_unlock_threads (cherokee_server_t *srv)
{
	ret_t   ret;
	list_t *i;

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


void
cherokee_server_step (cherokee_server_t *srv)
{
	ret_t ret;

	/* Get the time
	 */
	update_bogo_now (srv);
	ret = cherokee_thread_step (srv->main_thread, true);

	/* Logger flush 
	 */
	if (srv->log_flush_next < srv->bogo_now) {
		flush_logs (srv);
		srv->log_flush_next = srv->bogo_now + srv->log_flush_elapse;
	}

#ifndef CHEROKEE_EMBEDDED
	/* Clean IO cache
	 */
	if (srv->iocache_clean_next < srv->bogo_now) {
		cherokee_iocache_clean_up (srv->iocache, IOCACHE_BASIC_SIZE);	
		srv->iocache_clean_next = srv->bogo_now + IOCACHE_DEFAULT_CLEAN_ELAPSE;
	}
#endif

	/* Wanna exit?
	 */
	if (srv->wanna_exit) {
		cherokee_server_reinit (srv);
	}
}


static ret_t
config_module_execute_function (cherokee_server_t *srv, char *param, char *func_name)
{
#ifdef CHEROKEE_EMBEDDED
	return cherokee_embedded_read_config (srv);
#else
	ret_t   ret;
	ret_t (*read_config) (cherokee_server_t *srv, char *config_string);

	/* Load the module
	 */
	ret = cherokee_module_loader_load_no_global (&srv->loader, "read_config");
	if (ret != ret_ok) return ret_error;

	/* Get the function
	 */
	ret = cherokee_module_loader_get_sym (&srv->loader, "read_config", 
					      func_name,
					      (void **)&read_config);
	if (ret != ret_ok) return ret;

	/* Execute it!
	 */
	ret = read_config (srv, param);

	/* Clean up
	 */
	cherokee_module_loader_unload (&srv->loader, "read_config");

	return ret;
#endif
}


ret_t 
cherokee_server_read_config_file (cherokee_server_t *srv, char *path)
{
 	if (path == NULL) { 
 		path = CHEROKEE_CONFDIR"/cherokee.conf"; 
 	} 
	
	return config_module_execute_function (srv, path, "read_config_file");
}


ret_t 
cherokee_server_read_config_string (cherokee_server_t *srv, char *config_string)
{
	return config_module_execute_function (srv, config_string, "read_config_string");
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
cherokee_server_get_active_conns (cherokee_server_t *srv, int *num)
{
	int     active = 0;
	list_t *thread;

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
cherokee_server_get_reusable_conns (cherokee_server_t *srv, int *num)
{
	int     reusable = 0;
	list_t *thread, *i;

	/* Reusable connections
	 */
	list_for_each (thread, &srv->thread_list) {
		list_for_each (i, &THREAD(thread)->reuse_list) {
			reusable++;
		}
	}
	list_for_each (i, &THREAD(srv->main_thread)->reuse_list) {
		reusable++;
	}

	/* Return out parameters
	 */
	*num = reusable;
	return ret_ok;
}


ret_t 
cherokee_server_get_total_traffic (cherokee_server_t *srv, size_t *rx, size_t *tx)
{
	list_t *i;

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
	srv->wanna_exit      = true;

	return ret_ok;
}


ret_t
cherokee_server_handle_panic (cherokee_server_t *srv)
{
	int                re;
	cherokee_buffer_t *cmd;

	PRINT_ERROR_S ("Cherokee feels panic!\n");
	
	if ((srv == NULL) || (srv->panic_action == NULL)) {
		goto fin;
	}

	cherokee_buffer_new (&cmd);
	cherokee_buffer_add_va (cmd, "%s %d", srv->panic_action, getpid());

	re = system (cmd->buf);
	if (re < 0) {
#ifdef WEXITSTATUS		
		int val = WEXITSTATUS(re);
#else
		int val = re;			
#endif
		PRINT_ERROR ("PANIC: re-panic: '%s', status %d\n", cmd->buf, val);
	}

	cherokee_buffer_free (cmd);

fin:
	abort();
}


ret_t 
cherokee_server_del_connection (cherokee_server_t *srv, char *id_str)
{
	list_t *t, *c;
	culong_t id;
	
	id = strtol (id_str, NULL, 10);

	list_for_each (t, &srv->thread_list) {
		list_for_each (c, &THREAD(t)->active_list) {
			cherokee_connection_t *conn = CONN(c);

			if (conn->id == id) {
				conn->phase = phase_lingering;
				return ret_ok;
			}
		}
	}

	return ret_not_found;
}


ret_t 
cherokee_server_set_backup_mode (cherokee_server_t *srv, cherokee_boolean_t active)
{
	ret_t   ret;
	list_t *i;

	ret = cherokee_logger_set_backup_mode (srv->vserver_default->logger, active);
	if (unlikely (ret != ret_ok)) return ret;

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
	list_t *i;

	*active = false;

	cherokee_logger_get_backup_mode (srv->vserver_default->logger, active);
	if (*active == true) return ret_ok;

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
	FILE *file;
	CHEROKEE_TEMP(buffer, 10);

	if (cherokee_buffer_is_empty (&srv->pidfile))
		return ret_not_found;

	file = fopen (srv->pidfile.buf, "w");
	if (file == NULL) {
		PRINT_MSG ("ERROR: Can't write PID file '%s': %s\n", srv->pidfile.buf, strerror(errno));
		return ret_error;
	}

	snprintf (buffer, buffer_size, "%d\n", getpid());
	fwrite (buffer, 1, strlen(buffer), file);
	fclose (file);

	return ret_ok;
}
