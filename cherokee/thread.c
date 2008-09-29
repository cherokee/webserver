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
#include "thread.h"

#include <signal.h>
#include <errno.h>

#include "socket.h"
#include "server.h"
#include "server-protected.h"
#include "fdpoll.h"
#include "fdpoll-protected.h"
#include "connection.h"
#include "connection-protected.h"
#include "handler_error.h"
#include "header.h"
#include "header-protected.h"
#include "util.h"
#include "fcgi_manager.h"
#include "bogotime.h"


#define DEBUG_BUFFER(b)  fprintf(stderr, "%s:%d len=%d crc=%d\n", __FILE__, __LINE__, b->len, cherokee_buffer_crc32(b))
#define ENTRIES "core,thread"

static ret_t reactive_conn_from_polling  (cherokee_thread_t *thd, cherokee_connection_t *conn);


static void
thread_update_bogo_now (cherokee_thread_t *thd)
{
	/* Has it changed?
	 */
	if (thd->bogo_now == cherokee_bogonow_now)
		return;

	/* Update time_t
	 */
	thd->bogo_now = cherokee_bogonow_now;

	/* Update struct tm
	 */
	cherokee_bogotime_lock_read();

	memcpy (&thd->bogo_now_tmgmt, &cherokee_bogonow_tmgmt, sizeof(struct tm));
	memcpy (&thd->bogo_now_tmloc, &cherokee_bogonow_tmloc, sizeof(struct tm));

	/* Update cherokee_buffer_t
	 */
	cherokee_buffer_clean (&thd->bogo_now_strgmt);
	cherokee_buffer_add_buffer (&thd->bogo_now_strgmt, &cherokee_bogonow_strgmt);

	cherokee_bogotime_release();
}


#ifdef HAVE_PTHREAD       
static void *
thread_routine (void *data)
{
	cherokee_thread_t *thread = THREAD(data);

	/* Wait to start working
	 */
	CHEROKEE_MUTEX_LOCK (&thread->starting_lock);

	/* Update bogonow before start working
	 */
	thread_update_bogo_now (thread);

	/* Step, step, step, ..
	 */
	while (thread->exit == false) {
		cherokee_thread_step_MULTI_THREAD (thread, false);
	}

	pthread_exit (NULL);
}
#endif


ret_t 
cherokee_thread_unlock (cherokee_thread_t *thd)
{
#ifdef HAVE_PTHREAD
	CHEROKEE_MUTEX_UNLOCK (&thd->starting_lock);
#endif
	return ret_ok;
}


ret_t 
cherokee_thread_wait_end (cherokee_thread_t *thd)
{
	/* Wait until the thread exits
	 */
	CHEROKEE_THREAD_JOIN (thd->thread);
	return ret_ok;	
}


ret_t 
cherokee_thread_new  (cherokee_thread_t      **thd, 
		      void                   *server,
		      cherokee_thread_type_t  type, 
		      cherokee_poll_type_t    fdpoll_type,
		      cint_t                  system_fd_num,
		      cint_t                  fd_num,
		      cint_t                  conns_max,
		      cint_t                  keepalive_max)
{
	ret_t              ret;
	cherokee_server_t *srv = SRV(server);
	CHEROKEE_CNEW_STRUCT (1, n, thread);

	/* Init
	 */
	INIT_LIST_HEAD (LIST(&n->base));
	INIT_LIST_HEAD (LIST(&n->active_list));
	INIT_LIST_HEAD (LIST(&n->reuse_list));
	INIT_LIST_HEAD (LIST(&n->polling_list));

	n->exit                = false;
	n->server              = server;
	n->thread_type         = type;

	n->conns_num           = 0;
	n->conns_max           = conns_max;
	n->conns_keepalive_max = keepalive_max;

	n->active_list_num     = 0;
	n->polling_list_num    = 0;
	n->reuse_list_num      = 0;

	n->pending_conns_num   = 0;
	n->pending_read_num    = 0;

	n->fastcgi_servers     = NULL;
	n->fastcgi_free_func   = NULL;

	/* Event poll object
	 */
	if (fdpoll_type == cherokee_poll_UNSET)
		ret = cherokee_fdpoll_best_new (&n->fdpoll, system_fd_num, fd_num);
	else
		ret = cherokee_fdpoll_new (&n->fdpoll, fdpoll_type, system_fd_num, fd_num);

	if (unlikely (ret != ret_ok)) {
		CHEROKEE_FREE(n);
		return ret;
	}

	/* Sanity check */
	if (fd_num < conns_max) {
		cherokee_fdpoll_free (n->fdpoll);
		CHEROKEE_FREE (n);
		return ret_error;
	}

	/* Bogo now stuff
	 */
	n->bogo_now = 0;
	memset (&n->bogo_now_tmgmt, 0, sizeof (struct tm));
	cherokee_buffer_init (&n->bogo_now_strgmt);

	/* Temporary buffer used by utility functions
	 */
	cherokee_buffer_init (&n->tmp_buf1);
	cherokee_buffer_init (&n->tmp_buf2);
	cherokee_buffer_ensure_size (&n->tmp_buf1, 4096);	
	cherokee_buffer_ensure_size (&n->tmp_buf2, 4096);	

	/* Accepting information
	 */
	n->accept.recalculate    = 0;
	n->accept.continuous     = 0;
	n->accept.continuous_max = 0;

	/* The thread must adquire this mutex before 
	 * process its connections
	 */
	CHEROKEE_MUTEX_INIT (&n->ownership, NULL);
	
	/* Do some related work..
	 */
	if (type == thread_async) {
#ifdef HAVE_PTHREAD
		pthread_attr_t attr;
		
		/* Init the thread attributes
		 */
		pthread_attr_init (&attr);
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
		
		/* Maybe set the scheduling policy
		 */
		if (srv->thread_policy != -1) {
# ifdef HAVE_PTHREAD_SETSCHEDPOLICY
			pthread_attr_setschedpolicy (&attr, srv->thread_policy);
# endif
		}

		/* Set the start lock
		 */
		CHEROKEE_MUTEX_INIT (&n->starting_lock, NULL);
		CHEROKEE_MUTEX_LOCK (&n->starting_lock);

		/* Finally, create the system thread
		 */
		if (pthread_create (&n->thread, &attr, thread_routine, n) != 0) {
			cherokee_thread_free (n);
			return ret_error;
		}
#else
		SHOULDNT_HAPPEN;
#endif		
	}

	/* Return the object
	 */
	*thd = n;
	return ret_ok;
}


static void
conn_set_mode (cherokee_thread_t *thd, cherokee_connection_t *conn, cherokee_socket_status_t s)
{
	cherokee_socket_set_status (&conn->socket, s);
	cherokee_fdpoll_set_mode (thd->fdpoll, SOCKET_FD(&conn->socket), s);
}


static void
add_connection (cherokee_thread_t *thd, cherokee_connection_t *conn)
{
	cherokee_list_add_tail (LIST(conn), &thd->active_list);
	thd->active_list_num++;
}

static void
add_connection_polling (cherokee_thread_t *thd, cherokee_connection_t *conn)
{
	cherokee_list_add_tail (LIST(conn), &thd->polling_list);
	thd->polling_list_num++;
}

static void
del_connection (cherokee_thread_t *thd, cherokee_connection_t *conn)
{
	cherokee_list_del (LIST(conn));
	thd->active_list_num--;
}

static void
del_connection_polling (cherokee_thread_t *thd, cherokee_connection_t *conn)
{
	cherokee_list_del (LIST(conn));
	thd->polling_list_num--;
}


static ret_t
connection_reuse_or_free (cherokee_thread_t *thread, cherokee_connection_t *conn)
{
	/* Disable keepalive in the connection
	 */
	conn->keepalive = 0;
	
	/* Check the max connection reuse number
	 */
	if (thread->reuse_list_num >= THREAD_SRV(thread)->conns_reuse_max) {
		return cherokee_connection_free (conn);
	}

	/* Add it to the reusable connection list
	 */
	cherokee_list_add (LIST(conn), &thread->reuse_list);
	thread->reuse_list_num++;

	return ret_ok;
}


static void
purge_connection (cherokee_thread_t *thread, cherokee_connection_t *conn)
{
	/* Try last read, if previous read/write returned eof, then no
	 * problem, otherwise it may avoid a nasty connection reset.
	 */
	if (conn->phase == phase_lingering) {
		cherokee_connection_linger_read (conn);
	}

	/* It maybe have a delayed log
	 */
	cherokee_connection_log_delayed (conn);

	/* Close & clean the socket and clean up the connection object
	 */
	cherokee_connection_clean_close (conn);

	if (thread->conns_num > 0)
		thread->conns_num--;

	/* Add it to the reusable list
	 */	
	connection_reuse_or_free (thread, conn);
}


static cherokee_boolean_t 
check_addition_multiple_fd (cherokee_thread_t *thread, int fd)
{
	cherokee_list_t       *i;
	cherokee_connection_t *iconn;
	
	list_for_each (i, &thread->polling_list) {
		iconn = CONN(i);
		
		if (iconn->polling_fd == fd)
			return false;
	}
	return true;
}

static cherokee_boolean_t 
check_removal_multiple_fd (cherokee_thread_t *thread, int fd)
{
	cherokee_list_t       *i;
	cherokee_connection_t *iconn;
	cherokee_boolean_t     first = false;
	
	list_for_each (i, &thread->polling_list) {
		iconn = CONN(i);
		
		if (iconn->polling_fd == fd) {
			if (!first) {
				first = true;
				continue;
			}
			return false;
		}
	}
	return true;
}



static void
purge_closed_polling_connection (cherokee_thread_t *thread, cherokee_connection_t *conn)
{
	ret_t              ret;
	cherokee_boolean_t del_fd = true;

	/* Delete from file descriptors poll
	 */
	if (conn->polling_multiple) 
		del_fd = check_removal_multiple_fd (thread, conn->polling_fd);

	if (del_fd) {
		ret = cherokee_fdpoll_del (thread->fdpoll, conn->polling_fd);
		if (ret != ret_ok)
			SHOULDNT_HAPPEN;
	}

	/* Remove from the polling list
	 */
	del_connection_polling (thread, conn);
	
	/* The connection hasn't the main fd in the file descriptor poll
	 * so, we just have to remove the connection.
	 */
	purge_connection (thread, conn);
}


static void
purge_closed_connection (cherokee_thread_t *thread, cherokee_connection_t *conn)
{
	ret_t ret;

	/* Delete from file descriptors poll
	 */
	ret = cherokee_fdpoll_del (thread->fdpoll, SOCKET_FD(&conn->socket));
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	/* Remove from active connections list
	 */
	del_connection (thread, conn);

	/* Finally, purge connection
	 */
	purge_connection (thread, conn);
}


static void
purge_maybe_lingering (cherokee_thread_t *thread, cherokee_connection_t *conn)
{
	ret_t ret;

	if (conn->keepalive <= 0) {
		conn->phase = phase_lingering;
		purge_closed_connection (thread, conn);
		return;
	}

	/* Shutdown writing, and try to read some trash
	 */
	ret = cherokee_connection_shutdown_wr (conn);
	switch (ret) {
	case ret_ok:
	case ret_eagain:
		/* Ok, really lingering
		 */
		conn->phase = phase_lingering;
		conn_set_mode (thread, conn, socket_reading);
		return;
	default:
		/* Error: no linger and no last read, just close it
		 */
		purge_closed_connection (thread, conn);
		return;
	}	
}


static void
maybe_purge_closed_connection (cherokee_thread_t *thread, cherokee_connection_t *conn)
{
	ret_t              ret;
	cherokee_server_t *srv = SRV(thread->server);

	/* Log if it was delayed and update vserver traffic counters
	 */
	cherokee_connection_update_vhost_traffic (conn);
	cherokee_connection_log_delayed (conn);

	/* If it isn't a keep-alive connection, it should try to
	 * perform a lingering close (there is no need to disable TCP
	 * cork before shutdown or before a close).
	 */
	if (conn->keepalive <= 0) {
		ret = cherokee_connection_shutdown_wr (conn);
		switch (ret) {
		case ret_ok:
		case ret_eagain:
			/* Ok, lingering
			 */
			conn->phase = phase_lingering;
			conn_set_mode (thread, conn, socket_reading);
			return;
		default:
			/* Error, no linger and no last read, just
			 * close the connection.
			 */
			purge_closed_connection (thread, conn);
			return;
		}
	} 	

	conn->keepalive--;

	/* Clean the connection
	 */
	cherokee_connection_clean (conn);
	conn_set_mode (thread, conn, socket_reading);

	/* Flush any buffered data
	 */
	cherokee_socket_flush (&conn->socket);

	/* Update the timeout value
	 */
	conn->timeout = thread->bogo_now + srv->timeout;	
}


static ret_t 
process_polling_connections (cherokee_thread_t *thd)
{
	int                    re;
	cherokee_list_t       *tmp, *i;
	cherokee_connection_t *conn;

	list_for_each_safe (i, tmp, LIST(&thd->polling_list)) {
		conn = CONN(i);

		/* Has it been too much without any work?
		 */
		if (conn->timeout < thd->bogo_now) {
			TRACE (ENTRIES",polling", "conn %p(fd=%d): Time out\n", 
			       conn, SOCKET_FD(&conn->socket));
			purge_closed_polling_connection (thd, conn);
			continue;
		}
		
		/* Is there information to be sent?
		 */
		if (conn->buffer.len > 0) {
			reactive_conn_from_polling (thd, conn);
			continue;
		}

		/* Check the "extra" file descriptor
		 */
		re = cherokee_fdpoll_check (thd->fdpoll, conn->polling_fd, conn->polling_mode);
		switch (re) {
		case -1:
			/* Error, move back the connection
			 */
			TRACE (ENTRIES",polling", "conn %p(fd=%d): status is Error\n", 
			       conn, SOCKET_FD(&conn->socket));

			purge_closed_polling_connection (thd, conn);
			continue;
		case 0:			
			/* Nothing to do.. wait longer
			 */
			continue;
		}

		/* Move from the 'polling' to the 'active' list:
		 */
		reactive_conn_from_polling (thd, conn);
	}

	return ret_ok;
}


static ret_t 
process_active_connections (cherokee_thread_t *thd)
{
	int                    re;
	ret_t                  ret;
	off_t                  len;
	cherokee_list_t       *i, *tmp;
	cuint_t                conns_freed = 0;
	cherokee_connection_t *conn        = NULL;
	cherokee_server_t     *srv         = SRV(thd->server);

	/* Process active connections
	 */
	list_for_each_safe (i, tmp, LIST(&thd->active_list)) {
		conn = CONN(i);

		TRACE (ENTRIES, "thread (%p) processing conn (%p), phase %d '%s'\n", 
		       thd, conn, conn->phase, cherokee_connection_get_phase_str (conn));

		/* Has the connection been too much time w/o any work
		 */
		if (conn->timeout < thd->bogo_now) {
			TRACE (ENTRIES, "thread (%p) processing conn (%p): Time out\n", thd, conn);

			conns_freed++;
			purge_closed_connection (thd, conn);
			continue;
		}

		/* Maybe update traffic counters
		 */
		if ((conn->traffic_next < thd->bogo_now) &&
		    ((conn->rx != 0) || (conn->tx != 0)))
		{
			cherokee_connection_update_vhost_traffic (conn);
		}

		/* Process the connection?
		 * 2.- Inspect the file descriptor if it's not shutdown
		 *     and it's not reading header or there is no more buffered data.
		 */
		if ((! (conn->options & conn_op_was_polling)) &&
		    (conn->phase != phase_shutdown) &&
		    (conn->phase != phase_lingering) &&
		    (conn->phase != phase_reading_header || conn->incoming_header.len <= 0))
		{
			re = cherokee_fdpoll_check (thd->fdpoll, 
						    SOCKET_FD(&conn->socket), 
						    conn->socket.status);
			switch (re) {
			case -1:
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			case 0:
				if (! cherokee_socket_pending_read (&conn->socket))
					continue;
			}
		}

		/* This connection was polling a moment ago
		 */
		if (conn->options & conn_op_was_polling) {
			BIT_UNSET (conn->options, conn_op_was_polling);
		}

		/* Update the connection timeout
		 */
		if (conn->phase != phase_reading_header) {
			conn->timeout = thd->bogo_now + srv->timeout;
		}

		TRACE (ENTRIES, "conn on phase n=%d: %s\n", 
		       conn->phase, cherokee_connection_get_phase_str (conn));

		/* Phases
		 */
		switch (conn->phase) {
		case phase_switching_headers:
			ret = cherokee_connection_send_switching (conn);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eagain:
				continue;
				
			case ret_eof:
			case ret_error:
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
				
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			}
			conn->phase = phase_tls_handshake;;
			
		case phase_tls_handshake:
			ret = cherokee_socket_init_tls (&conn->socket, CONN_VSRV(conn));

			switch (ret) {

			case ret_eagain:
				continue;
				
			case ret_ok:
				/* RFC2817
				 * Had it upgraded the protocol?
				 */
				if (conn->error_code == http_switching_protocols) {
					conn->phase = phase_setup_connection;
					conn->error_code = http_ok;
					continue;
				}
				conn->phase = phase_reading_header;
				break;

			case ret_error:
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
				
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				purge_closed_connection (thd, conn);
				break;
			}
			break;

		case phase_read_post:
			len = 0;
			ret = cherokee_connection_recv (conn, POST_BUF(&conn->post), &len);
			
			switch (ret) {
			case ret_eagain:
				continue;

			case ret_ok:
				cherokee_post_commit_buf (&conn->post, len);
				if (cherokee_post_got_all (&conn->post)) {	
					break;
				}
				continue;
				
			case ret_eof:
				/* Finish..
				 */
				if (!cherokee_post_got_all (&conn->post)) {
					conns_freed++;
					purge_closed_connection (thd, conn);
					continue;
				}

				cherokee_post_commit_buf (&conn->post, len);
				break;

			case ret_error:
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
				
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			}
			
			/* Turn the connection in write mode
			 */
			conn_set_mode (thd, conn, socket_writing);
			conn->phase = phase_setup_connection;
			break;


		case phase_reading_header: 
			/* Maybe the buffer has a request (previous pipelined)
			 */
			if (! cherokee_buffer_is_empty (&conn->incoming_header))
			{
				ret = cherokee_header_has_header (&conn->header,
								  &conn->incoming_header, 
								  conn->incoming_header.len);
				switch (ret) {
				case ret_ok:
					goto phase_reading_header_EXIT;
				case ret_not_found:
					break;
				case ret_error:
					conns_freed++;
					purge_closed_connection (thd, conn);
					continue;
				default:
					RET_UNKNOWN(ret);
					conns_freed++;
					purge_closed_connection (thd, conn);
					continue;
				}
			}

			/* Read from the client
			 */
			ret = cherokee_connection_recv (conn, &conn->incoming_header, &len);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eagain:
				continue;
			case ret_eof:
			case ret_error:
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			}

			/* Check security after read
			 */
			ret = cherokee_connection_reading_check (conn);
			if (ret != ret_ok) {
				conn->keepalive      = 0; 
				conn->phase          = phase_setup_connection;
				conn->header.version = http_version_11;
				continue;
			}

			/* May it already has the full header
			 */
			ret = cherokee_header_has_header (&conn->header, &conn->incoming_header, len+4);
			switch (ret) {
			case ret_ok:
				break;
			case ret_not_found:
				conn->phase = phase_reading_header;
				continue;
			case ret_error:
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			}

			/* fall down */

		phase_reading_header_EXIT:
			conn->phase = phase_processing_header;

			/* fall down */

		case phase_processing_header:
			/* Get the request
			 */
			ret = cherokee_connection_get_request (conn);
			switch (ret) {
			case ret_ok:
				break;

			case ret_eagain:
				continue;

			default:
				cherokee_connection_setup_error_handler (conn);
				conn_set_mode (thd, conn, socket_writing);
				conn->phase = phase_init;
				continue;
			}

			/* If it's a POST we've to read more data
			 */
			if (http_method_with_input (conn->header.method)) {
				if (! cherokee_post_got_all (&conn->post)) {
					conn_set_mode (thd, conn, socket_reading);
					conn->phase = phase_read_post;
					continue;
				}
			}
			
			conn->phase = phase_setup_connection;

			/* fall down */
			
		case phase_setup_connection: {
			cherokee_config_entry_t  entry; 
			cherokee_rule_list_t    *rules;
			cherokee_boolean_t       is_userdir;
			
			TRACE (ENTRIES, "Setup connection begins: request=\"%s\"\n", conn->request.buf);
			TRACE_CONN(conn);
		     
			/* Turn the connection in write mode
			 */
			conn_set_mode (thd, conn, socket_writing);
			
			/* Set the logger of the connection
			 */
			conn->logger_ref = CONN_VSRV(conn)->logger;

			/* Is it already an error response?
			 */
			if (http_type_300(conn->error_code) ||
			    http_type_400(conn->error_code) ||
			    http_type_500(conn->error_code)) 
			{
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}

			cherokee_config_entry_init (&entry);

			/* Choose the virtual entries table
			 */
			is_userdir = ((CONN_VSRV(conn)->userdir.len > 0) && (conn->userdir.len > 0));

			if (is_userdir) {
				rules = &CONN_VSRV(conn)->userdir_rules;
			} else {
				rules = &CONN_VSRV(conn)->rules;
			}
			
			/* Check against the rule list
			 */
			ret = cherokee_rule_list_match (rules, conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}

			if (cherokee_buffer_is_empty (&conn->local_directory)) {
				if (is_userdir)
					ret = cherokee_connection_build_local_directory_userdir (conn, CONN_VSRV(conn), &entry);
				else
					ret = cherokee_connection_build_local_directory (conn, CONN_VSRV(conn), &entry);
			}

			/* Check of the HTTP method is supported by the handler
			 */
			ret = cherokee_connection_check_http_method (conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}			

			/* Check Only-Secure connections
			 */
			ret = cherokee_connection_check_only_secure (conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}			

			/* Check for IP validation
			 */
			ret = cherokee_connection_check_ip_validation (conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}

			/* Check for authentication
			 */
			ret = cherokee_connection_check_authentication (conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}
			
			/* Update the keep-alive property
			 */
			cherokee_connection_set_keepalive (conn);

			/* Create the handler
			 */
			ret = cherokee_connection_create_handler (conn, &entry);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eagain:
				cherokee_connection_clean_for_respin (conn);
				continue;
			default:
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}
			
			/* Instance a encoded if needed
			 */
			ret = cherokee_connection_create_encoder (conn, &srv->encoders, entry.encoders);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}

			/* Parse the rest of headers
			 */
			ret = cherokee_connection_parse_range (conn);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}

			conn->phase = phase_init;			

			/* There isn't need of free entry, it is in the stack and the
			 * buffers inside it are just references..
			 */
		}
			
		case phase_init: 
			/* Look for the request
			 */
			ret = cherokee_connection_open_request (conn);
			switch (ret) {
			case ret_ok:
			case ret_error:
				break;

			case ret_eagain:
				continue;

			default:
				if ((MODULE(conn->handler)->info) &&
				    (MODULE(conn->handler)->info->name)) 
					PRINT_ERROR ("Unknown ret %d from handler %s\n", ret,
						     MODULE(conn->handler)->info->name);
				else
					RET_UNKNOWN(ret);
				break;
			}
			
			/* If it is an error, and the connection has not a handler to manage
			 * this error, the handler has to be changed by an error_handler.
			 */
			if (conn->handler == NULL) {
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			}

 			if (http_type_300(conn->error_code) || 
			    http_type_400(conn->error_code) ||
			    http_type_500(conn->error_code))
			{
				if (HANDLER_SUPPORTS (conn->handler, hsupport_error)) {
					ret = cherokee_connection_clean_error_headers (conn);
					if (unlikely (ret != ret_ok)) {
						conns_freed++;
						purge_closed_connection (thd, conn);
						continue;
					}
				} else {
					/* Try to setup an error handler
					 */
					ret = cherokee_connection_setup_error_handler (conn);
					if (ret != ret_ok) {
					
						/* It could not change the handler to an error
						 * managing handler, so it is a critical error.
						 */					
						conns_freed++;
						purge_closed_connection (thd, conn);
						continue;
					}

					/* At this point, two different things might happen:
					 * - It has got a common handler like handler_redir
					 * - It has got an error handler like handler_error
					 */
					conn->phase = phase_init;
					break;
				}
			}
			
			conn->phase = phase_add_headers;

		case phase_add_headers:
			/* Build the header
			 */
			ret = cherokee_connection_build_header (conn);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eagain:
				continue;
			case ret_eof:
			case ret_error:
				cherokee_connection_setup_error_handler (conn);
				conn->error_code = http_internal_error;
				conn->phase = phase_init;
				continue;
			default:
				RET_UNKNOWN(ret);
			}

			/* If it is an error, we have to respin the connection
			 * to install a proper error handler.
			 */
			if ((http_type_300 (conn->error_code) || 
			     http_type_400 (conn->error_code) ||
			     http_type_500 (conn->error_code)) &&
			    (!HANDLER_SUPPORTS (conn->handler, hsupport_error))) {
				conn->phase = phase_setup_connection;
				continue;				
			}

			/* If it has mmaped content, skip next stage
			 */		     
			if (conn->mmaped != NULL)
				goto phase_send_headers_EXIT;

			conn->phase = phase_send_headers;

		case phase_send_headers:

			/* Send headers to the client
			 */
			ret = cherokee_connection_send_header (conn);
			switch (ret) {
			case ret_eagain:
				continue;

			case ret_ok:
				if (!http_method_with_body (conn->header.method)) {
					maybe_purge_closed_connection (thd, conn);
					continue;
				}
				if (!http_code_with_body (conn->error_code)) {
					maybe_purge_closed_connection (thd, conn);
					continue;
				}
				break;

			case ret_eof:
			case ret_error:
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;

			default:
				RET_UNKNOWN(ret);
			}

			/* Maybe log the connection
			 */
			cherokee_connection_log_or_delay (conn);			

		phase_send_headers_EXIT:
			conn->phase = phase_steping;
			

		case phase_steping: 
			/* Special case: 
			 * If the content is mmap()ed, it has to send the header +
			 * the file content and stop processing the connection.
			 */
			if (conn->mmaped != NULL) {
				ret = cherokee_connection_send_header_and_mmaped (conn);
				switch (ret) {
				case ret_eagain:
					continue;

				case ret_eof:
					maybe_purge_closed_connection (thd, conn);
					continue;

				case ret_error:
					purge_maybe_lingering (thd, conn);
					continue;

				default:
					maybe_purge_closed_connection (thd, conn);
					continue;
				}
			}

			/* Handler step: read or make new data to send
			 */
			ret = cherokee_connection_step (conn);
			switch (ret) {
			case ret_eagain:
				break;

			case ret_eof_have_data:
				ret = cherokee_connection_send (conn);

				switch (ret) {
				case ret_ok:
					maybe_purge_closed_connection (thd, conn);
					continue;
				case ret_eagain:
					break;
				case ret_eof:
				case ret_error:
				default:	
					conns_freed++;
					purge_closed_connection (thd, conn);
					continue;
				}
				break;

			case ret_ok:
				ret = cherokee_connection_send (conn);

				switch (ret) {
				case ret_ok:
					continue;
				case ret_eagain:
					break;
				case ret_eof:
				case ret_error:
				default:
					conns_freed++;
					purge_closed_connection (thd, conn);
					continue;
				}
				break;

			case ret_ok_and_sent:
				break;

			case ret_eof:
				maybe_purge_closed_connection (thd, conn);
				continue;
			case ret_error:
				purge_maybe_lingering (thd, conn);
				continue;

			default:
				RET_UNKNOWN(ret);
				purge_maybe_lingering (thd, conn);
			}
			break;
			
		case phase_shutdown: 
			ret = cherokee_connection_shutdown_wr (conn);
			switch (ret) {
			case ret_ok:
			case ret_eagain:
				/* Ok, really lingering
				 */
				conn->phase = phase_lingering;
				conn_set_mode (thd, conn, socket_reading);
				break;
			default:
				/* Error, no linger and no last read,
				 * just close the connection.
				 */
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			}
			/* fall down */

		case phase_lingering: 
			ret = cherokee_connection_linger_read (conn);
			switch (ret) {
			case ret_ok:
			case ret_eagain:
				continue;
			case ret_eof:
			case ret_error:
				conns_freed++;
				purge_closed_connection (thd, conn);
				continue;
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				purge_closed_connection (thd, conn);
				break;
			}
			break;

		default:
 			SHOULDNT_HAPPEN;
		}

	} /* list */	

	return ret_ok;
}


ret_t 
cherokee_thread_free (cherokee_thread_t *thd)
{
	cherokee_list_t *i, *tmp;

	cherokee_buffer_mrproper (&thd->bogo_now_strgmt);
	cherokee_buffer_mrproper (&thd->tmp_buf1);
	cherokee_buffer_mrproper (&thd->tmp_buf2);

	cherokee_fdpoll_free (thd->fdpoll);
	thd->fdpoll = NULL;

	/* Free the connection
	 */
	list_for_each_safe (i, tmp, &thd->active_list) {
		cherokee_connection_free (CONN(i));
	}

	list_for_each_safe (i, tmp, &thd->reuse_list) {
		cherokee_connection_free (CONN(i));
	}

	/* FastCGI
	 */
	if (thd->fastcgi_servers != NULL) {
		cherokee_avl_free (thd->fastcgi_servers, thd->fastcgi_free_func);
		thd->fastcgi_servers = NULL;
	}

	CHEROKEE_MUTEX_DESTROY (&thd->starting_lock);
	CHEROKEE_MUTEX_DESTROY (&thd->ownership);
	
	free (thd);
	return ret_ok;
}

static void
thread_full_handler (cherokee_thread_t *thd, int srv_socket)
{
	ret_t                ret;
	cherokee_list_t     *i;
	cherokee_socket_t    sock;
	size_t               read  = 0;
	cherokee_boolean_t   done  = false;
	cherokee_server_t   *srv   = THREAD_SRV(thd);
	cherokee_buffer_t   *tmp   = THREAD_TMP_BUF1(thd);

	/* Short path: nothing to accept
	 */
	if (cherokee_fdpoll_check (thd->fdpoll, srv_socket, FDPOLL_MODE_READ) <= 0) {
		return;
	}

	/* Check all the threads
	 */
	list_for_each (i, &srv->thread_list) {
		if (THREAD(i)->conns_num < THREAD(i)->conns_max)
			return;
	}

	if (srv->main_thread->conns_num < srv->main_thread->conns_max)
		return;

	/* In case there is no room in the entire server. The best
	 * thing we can do reached this point is to get rid of the
	 * connection as soon and quickly as possible.
	 */
	ret = cherokee_socket_init (&sock);
	if (unlikely (ret != ret_ok))
		goto out;

	/* Accept a connection
	 */
	do {
		ret = cherokee_socket_accept (&sock, srv_socket);
	} while (ret == ret_deny);

	if (ret != ret_ok)
		goto out;
	
	/* Read the request
	 */
#if 0
	do {
		ret = cherokee_socket_read (&sock, tmp->buf, tmp->size, &read);
	} while (ret == ret_eagain);
#endif
	
	/* Write the error response
	 */
	cherokee_buffer_clean (tmp);
	cherokee_buffer_add_str (
		tmp,
		"HTTP/1.0 " http_service_unavailable_string CRLF_CRLF	    \
		"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" CRLF \
		"<html><head><title>" http_service_unavailable_string	    \
		"</title></head><body><h1>" http_service_unavailable_string \
		"</h1><p>Server run out of resources.</p></body></html>");
	do {
		read = 0;

		ret = cherokee_socket_bufwrite (&sock, tmp, &read);
		switch (ret) {
		case ret_ok:
			if (read > 0) {
				cherokee_buffer_move_to_begin (tmp, read);
			}
		default:
			done = true;
		}
		
		if (cherokee_buffer_is_empty (tmp))
			done = true;
	} while (!done);

out:
	cherokee_socket_close (&sock);
	cherokee_socket_mrproper (&sock);
}

static int
accept_new_connection (cherokee_thread_t *thd, int srv_socket, cherokee_socket_type_t tls)
{
	ret_t                  ret;
	int                    new_fd;
	cherokee_sockaddr_t    new_sa;
	cherokee_connection_t *new_conn;

	/* Check whether there are connections waiting
	 */
	if (cherokee_fdpoll_check (thd->fdpoll, srv_socket, FDPOLL_MODE_READ) <= 0) {
		return 0;
	}

	/* Try to get a new connection
	 */
	do {
		ret = cherokee_socket_accept_fd (srv_socket, &new_fd, &new_sa);
	} while (ret == ret_deny);

	if (ret != ret_ok)
		return 0;

	/* We got the new socket, now set it up in a new connection object
	 */
	ret = cherokee_thread_get_new_connection (thd, &new_conn);
	if (unlikely(ret < ret_ok)) {
		PRINT_ERROR_S ("ERROR: Trying to get a new connection object\n");
		cherokee_fd_close (new_fd);
		return 0;
	}

	/* We got a new_conn object, on error we can goto error.
	 */
	ret = cherokee_socket_set_sockaddr (&new_conn->socket, new_fd, &new_sa);

	/* It is about to add a new connection to the thread, 
	 * so it MUST adquire the thread ownership
	 * (do it now to better handle error cases).
	 */
	CHEROKEE_MUTEX_LOCK (&thd->ownership);

	if (unlikely(ret < ret_ok)) {
		PRINT_ERROR_S ("ERROR: Trying to set sockaddr\n");
		goto error;
	}

	/* TLS support, set initial connection phase.
	 */
	if (tls == TLS) {
		new_conn->phase = phase_tls_handshake;
	}

	/* Lets add the new connection
	 */
	ret = cherokee_thread_add_connection (thd, new_conn);
	if (unlikely (ret < ret_ok))
		goto error;

	thd->conns_num++;

	/* Release the thread ownership
	 */
	CHEROKEE_MUTEX_UNLOCK (&thd->ownership);

	TRACE (ENTRIES, "new conn %p, fd %d\n", new_conn, new_fd);

	return 1;

error:
	TRACE (ENTRIES, "error accepting connection fd %d from fd %d\n", new_fd, srv_socket);

	/* Close new socket and reset its socket fd to default value.
	 */
	cherokee_fd_close (new_fd);
	SOCKET_FD(&new_conn->socket) = -1;

	/* Don't waste / reuse this new_conn object.
	 */
	connection_reuse_or_free (thd, new_conn);

	/* Release the thread ownership
	 */
	CHEROKEE_MUTEX_UNLOCK (&thd->ownership);
	return 0;
}


static int
should_accept_more (cherokee_thread_t *thd, int re)
{
	const uint32_t recalculate_steps = 10;

	/* If it is full, do not accept more!
	 */
	if (unlikely (thd->conns_num >= thd->conns_max))
		return 0;

	if (unlikely ((THREAD_SRV(thd)->wanna_reinit) ||
		      (THREAD_SRV(thd)->wanna_exit)))
		return 0;
#if 0
	if (unlikely (cherokee_fdpoll_is_full(thd->fdpoll))) {
		return 0;
	}
#endif

	/* Got new connection
	 */
	if (re > 0) {
		thd->accept.continuous++;

		if (thd->accept.recalculate <= 0) {
			thd->accept.continuous_max = thd->accept.continuous;
			return 1;
		}

		if (thd->accept.continuous > thd->accept.continuous_max) {
			thd->accept.continuous_max = thd->accept.continuous;
			thd->accept.recalculate--;
			return 0;
		}

		return 1;
	}

	/* Failed to get a new connection
	 */
	thd->accept.continuous  = 0;
	thd->accept.recalculate = recalculate_steps;
	return 0;
}


ret_t 
cherokee_thread_step_SINGLE_THREAD (cherokee_thread_t *thd)
{
	int                re;
	cherokee_boolean_t accepting;
	cherokee_server_t *srv           = THREAD_SRV(thd);
	int                fdwatch_msecs = srv->fdwatch_msecs;

	/* Try to update bogo_now
	 */
	cherokee_bogotime_try_update();

#if 0
	if (unlikely (cherokee_fdpoll_is_full (thd->fdpoll))) {
		goto out;
	}
#endif

	/* Graceful restart
	 */
	if (srv->wanna_reinit)
		goto out;

	/* If thread has pending connections, it should do a 
	 * faster 'watch' (whenever possible).
	 */
	if (thd->pending_conns_num > 0) {
		fdwatch_msecs          = 0;
		thd->pending_conns_num = 0;
	}

	if (thd->pending_read_num > 0) {
		fdwatch_msecs         = 0;
		thd->pending_read_num = 0;
	}

	re = cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	if (re <= 0)
		goto out;

	thread_update_bogo_now (thd);

	/* If the thread is full of connections, it should not
	 * get new connections.
	 */
	accepting = (thd->conns_num < thd->conns_max);
	if (accepting) {
		do {
			re = accept_new_connection (thd, S_SOCKET_FD(srv->socket), non_TLS);
		} while (should_accept_more (thd, re));
		
#ifdef HAVE_TLS
		if (srv->tls_enabled) {
			do {
				re = accept_new_connection (thd, S_SOCKET_FD(srv->socket_tls), TLS);
			} while (should_accept_more (thd, re));
		}
#endif
	} else {
		thread_full_handler (thd, S_SOCKET_FD(srv->socket));
		if (srv->tls_enabled) {
			thread_full_handler (thd, S_SOCKET_FD(srv->socket_tls));
		}
	}

out:
	thread_update_bogo_now (thd);

	/* Process polling connections
	 */
	process_polling_connections (thd);

	/* Process active connections
	 */
	return process_active_connections (thd);
}


#ifdef HAVE_PTHREAD

static ret_t
step_MULTI_THREAD_block (cherokee_thread_t *thd, int socket, pthread_mutex_t *mutex, int fdwatch_msecs)
{
	int                re;
	ret_t              ret;
	cherokee_boolean_t accepting;

	CHEROKEE_MUTEX_LOCK (mutex);
	
	ret = cherokee_fdpoll_add (thd->fdpoll, socket, FDPOLL_MODE_READ);
	if (unlikely (ret < ret_ok)) {
		CHEROKEE_MUTEX_UNLOCK (mutex);
		return ret_error;
	}

	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	thread_update_bogo_now (thd);

	/* Accept a new connection
	 */
	accepting = (thd->conns_num < thd->conns_max);
	if (accepting) {
		do {
			re = accept_new_connection (thd, socket, non_TLS);
		} while (should_accept_more (thd, re));
	} else {
		thread_full_handler (thd, socket);
	}

	/* Release the socket
	 */
	ret = cherokee_fdpoll_del (thd->fdpoll, socket);
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	CHEROKEE_MUTEX_UNLOCK (mutex);	
	return ret_ok;
}


static ret_t
step_MULTI_THREAD_nonblock (cherokee_thread_t *thd, int socket, pthread_mutex_t *mutex, int fdwatch_msecs)
{
	ret_t              ret;
	int                re;
	cherokee_boolean_t accepting;
	int                unlocked  = 1;

	/* Try to lock. Not success: wait for an event to happen
	 */
	unlocked = CHEROKEE_MUTEX_TRY_LOCK (mutex);
	if (unlocked) {
		cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
		return ret_ok;
	}

	/* Now it owns the socket
	 */
	ret = cherokee_fdpoll_add (thd->fdpoll, socket, FDPOLL_MODE_READ);
	if (unlikely (ret < ret_ok)) {
		ret = ret_error;
		goto out;
	}

	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	thread_update_bogo_now (thd);

	/* It should either accept o discard a connection
	 */
	accepting = (thd->conns_num < thd->conns_max);
	if (accepting) {
		do {
			re = accept_new_connection (thd, socket, non_TLS);
		} while (should_accept_more (thd, re));

	} else {
		thread_full_handler (thd, socket);
	}

	/* Release the server socket
	 */
	ret = cherokee_fdpoll_del (thd->fdpoll, socket);
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	ret = ret_ok;

out:
	CHEROKEE_MUTEX_UNLOCK (mutex);
	return ret;

}

# ifdef HAVE_TLS

static ret_t
step_MULTI_THREAD_TLS_nonblock (cherokee_thread_t *thd, int fdwatch_msecs, 
				int socket,     pthread_mutex_t *mutex, 
				int socket_tls, pthread_mutex_t *mutex_tls)
{
	ret_t              ret;
	int                re;
	cherokee_boolean_t accepting;
	int                unlocked     = 1;
	int                unlocked_tls = 1;

	/* Try to lock both mutexes
	 */
	unlocked = CHEROKEE_MUTEX_TRY_LOCK (mutex);
	if (! unlocked) {
		ret = cherokee_fdpoll_add (thd->fdpoll, socket, FDPOLL_MODE_READ);
		if (unlikely (ret < ret_ok)) {
			ret = ret_error;
			goto out;
		}
	}
	
	/* Try to lock the TLS mutex
	 */
	unlocked_tls = CHEROKEE_MUTEX_TRY_LOCK (mutex_tls);
	if (!unlocked_tls) {
		ret = cherokee_fdpoll_add (thd->fdpoll, socket_tls, FDPOLL_MODE_READ);
		if (unlikely (ret < ret_ok)) {
			ret = ret_error;
			goto out;
		}
	}

	/* Inspect the fds. It may sleep if nothing happens
	 */
	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	thread_update_bogo_now (thd);
		
	/* accept o discard a connections
	 */
	accepting = (thd->conns_num < thd->conns_max);
	if (accepting) {
		if (!unlocked) {
			do {
				re = accept_new_connection (thd, socket, non_TLS);
			} while (should_accept_more (thd, re));
		
			ret = cherokee_fdpoll_del (thd->fdpoll, socket);
			if (ret != ret_ok)
				SHOULDNT_HAPPEN;
		}
	
		if (!unlocked_tls) {
			do {
				re = accept_new_connection (thd, socket_tls, TLS);
			} while (should_accept_more (thd, re));
			
			ret = cherokee_fdpoll_del (thd->fdpoll, socket_tls);
			if (ret != ret_ok)
				SHOULDNT_HAPPEN;
		}

	} else {
		if (!unlocked)
			thread_full_handler (thd, socket);

		if (!unlocked_tls) 
			thread_full_handler (thd, socket_tls);
	}
	
	ret = ret_ok;

out:
	if (!unlocked)
		CHEROKEE_MUTEX_UNLOCK (mutex);
	if (!unlocked_tls)
		CHEROKEE_MUTEX_UNLOCK (mutex_tls);

	return ret;
}

static ret_t
step_MULTI_THREAD_TLS_block (cherokee_thread_t *thd, int fdwatch_msecs, 
			     int socket,     pthread_mutex_t *mutex, 
			     int socket_tls, pthread_mutex_t *mutex_tls)
{
	int                     re;
	ret_t                   ret;
	int                     socket1;
	int                     socket2;
	pthread_mutex_t        *mutex1;
	pthread_mutex_t        *mutex2;
	cherokee_socket_type_t  type1;
	cherokee_socket_type_t  type2;
	cherokee_boolean_t      accepting;

	if (thd->thread_pref == thread_tls_normal) {
		socket1 = socket;
		mutex1  = mutex;
		type1   = non_TLS;

		socket2 = socket_tls;
		mutex2  = mutex_tls;
		type2   = TLS;
	} else {
		socket1 = socket_tls;
		mutex1  = mutex_tls;
		type1   = TLS;

		socket2 = socket;
		mutex2  = mutex;
		type2   = non_TLS;
	}

	/* Lock the main mutex
	 */
	CHEROKEE_MUTEX_LOCK (mutex1);

	ret = cherokee_fdpoll_add (thd->fdpoll, socket1, FDPOLL_MODE_READ);
	if (ret < ret_ok) {
		CHEROKEE_MUTEX_UNLOCK (mutex1);
		return ret_error;
	}

	/* Try to lock the optional groups
	 */
#if 0
	unlock2 = CHEROKEE_MUTEX_TRY_LOCK (mutex2);
	if (!unlock2) {
		ret = cherokee_fdpoll_add (thd->fdpoll, socket2, FDPOLL_MODE_READ);
		if (ret < ret_ok) {
			CHEROKEE_MUTEX_UNLOCK (mutex1);
			CHEROKEE_MUTEX_UNLOCK (mutex2);
			return ret_error;
		}
	}
#endif

	/* Inspect the fds and get new connections
	 */
	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	thread_update_bogo_now (thd);
		
	/* Accept / Discard connection
	 */
	accepting = (thd->conns_num < thd->conns_max);
	if (accepting) {
		do {
			re = accept_new_connection (thd, socket1, type1);
		} while (should_accept_more (thd, re));
	} else {
		thread_full_handler (thd, socket1);		
	}

	/* Unlock the mail lock
	 */
	ret = cherokee_fdpoll_del (thd->fdpoll, socket1);
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	CHEROKEE_MUTEX_UNLOCK (mutex1);

	/* Maybe work with the optional socket
	 */
#if 0
	if (!unlock2) {
		do {
			re = accept_new_connection (thd, socket2, type2);
		} while (should_accept_more (thd, re));
		
		ret = cherokee_fdpoll_del (thd->fdpoll, socket2);
		if (ret != ret_ok)
			SHOULDNT_HAPPEN;

		CHEROKEE_MUTEX_UNLOCK (mutex2);
	}
#endif
	
	return ret_ok;
}
# endif /* HAVE_TLS */


ret_t 
cherokee_thread_step_MULTI_THREAD (cherokee_thread_t *thd, cherokee_boolean_t dont_block)
{
	ret_t              ret;
	cherokee_server_t *srv           = THREAD_SRV(thd);
	int                fdwatch_msecs = srv->fdwatch_msecs;

	/* Try to update bogo_now
	 */
	cherokee_bogotime_try_update();

#if 0
	if (unlikely (cherokee_fdpoll_is_full (thd->fdpoll))) {
		goto out;
	}
#endif

	/* Server wants to exit, and the thread has nothing to do
	 */
	if (unlikely (srv->wanna_exit)) {
		thd->exit = true;
		return ret_eof;
	}

	if (unlikely (srv->wanna_reinit))
	{
		if ((thd->active_list_num == 0) && 
		    (thd->polling_list_num == 0))
		{
			thd->exit = true;
			return ret_eof;
		}
		
		cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
		goto out;
	}

	/* If thread has pending connections, it should do a 
	 * faster 'watch' (whenever possible)
	 */
	if (thd->pending_conns_num > 0) {
		fdwatch_msecs          = 0;
		thd->pending_conns_num = 0;
	}

	if (thd->pending_read_num > 0) {
		fdwatch_msecs         = 0;
		thd->pending_read_num = 0;
	}

#ifdef HAVE_TLS
	/* Try to get new connections from https
	 */
	if (srv->tls_enabled) {
		if ((!dont_block) &&
		    (thd->exit == false) &&
		    (thd->active_list_num == 0) &&
		    (thd->polling_list_num == 0))
		{
			step_MULTI_THREAD_TLS_block (thd, fdwatch_msecs, 
			    S_SOCKET_FD(srv->socket),     &THREAD_SRV(thd)->accept_mutex, 	
			    S_SOCKET_FD(srv->socket_tls), &THREAD_SRV(thd)->accept_tls_mutex);
		} else {
			step_MULTI_THREAD_TLS_nonblock (thd, fdwatch_msecs, 
			    S_SOCKET_FD(srv->socket),     &THREAD_SRV(thd)->accept_mutex, 	
			    S_SOCKET_FD(srv->socket_tls), &THREAD_SRV(thd)->accept_tls_mutex);
		}
		
		goto out;
	}
#endif	

	/* Try to get new connections from http
	 */
	if ((!dont_block) &&
	    (thd->exit == false) &&
	    (thd->active_list_num == 0) && 
	    (thd->polling_list_num == 0))
	{
		step_MULTI_THREAD_block (thd, S_SOCKET_FD(srv->socket), 
					 &THREAD_SRV(thd)->accept_mutex, fdwatch_msecs);
	} else {
		step_MULTI_THREAD_nonblock (thd, S_SOCKET_FD(srv->socket),
					    &THREAD_SRV(thd)->accept_mutex, fdwatch_msecs);
	}
	
out:
	thread_update_bogo_now (thd);

	/* Adquire the ownership of the thread
	 */
	CHEROKEE_MUTEX_LOCK (&thd->ownership);

	/* Process polling connections
	 */
	process_polling_connections (thd);

	/* Process active connections
	 */
	ret = process_active_connections (thd);

	/* Release the thread
	 */
	CHEROKEE_MUTEX_UNLOCK (&thd->ownership);

	return ret;
}

#endif /* HAVE_PTHREAD */


ret_t 
cherokee_thread_get_new_connection (cherokee_thread_t *thd, cherokee_connection_t **conn)
{
	cherokee_connection_t *new_connection;
	cherokee_server_t     *server;
	static cuint_t         last_conn_id = 0;

	server = SRV(thd->server);

	if (cherokee_list_empty (&thd->reuse_list)) {
		ret_t ret;

		/* Create new connection object
		 */
		ret = cherokee_connection_new (&new_connection);
		if (unlikely(ret < ret_ok)) return ret;
	} else {
		/* Reuse an old one
		 */
		new_connection = CONN(thd->reuse_list.prev);
		cherokee_list_del (LIST(new_connection));
		thd->reuse_list_num--;

		INIT_LIST_HEAD (LIST(new_connection));		
	}

	/* Set the basic information to the connection
	 */
	new_connection->id        = last_conn_id++;
	new_connection->thread    = thd;
	new_connection->server    = server;
	new_connection->vserver   = VSERVER(server->vservers.prev); 

	new_connection->timeout   = thd->bogo_now + THREAD_SRV(thd)->timeout;

	*conn = new_connection;
	return ret_ok;
}


ret_t 
cherokee_thread_add_connection (cherokee_thread_t *thd, cherokee_connection_t  *conn)
{
	ret_t ret;

	ret = cherokee_fdpoll_add (thd->fdpoll, SOCKET_FD(&conn->socket), FDPOLL_MODE_READ);
	if (unlikely (ret < ret_ok)) return ret;

	conn_set_mode (thd, conn, socket_reading);
	add_connection (thd, conn);

	return ret_ok;
}


int   
cherokee_thread_connection_num (cherokee_thread_t *thd) 
{
	return thd->active_list_num;
}


ret_t 
cherokee_thread_close_polling_connections (cherokee_thread_t *thd, int fd, cuint_t *num)
{
	cuint_t                n = 0;
	cherokee_list_t       *i, *tmp;
	cherokee_connection_t *conn;

	list_for_each_safe (i, tmp, &thd->polling_list) {
		conn = CONN(i);

		if (conn->polling_fd == fd) {
			purge_closed_polling_connection (thd, conn);
			n++;
		}
	}

	if (num != NULL) *num = n;
	return ret_ok;
}


/* Interface for handlers:
 * It could want to add a file descriptor to the thread fdpoll
 */

static ret_t 
move_connection_to_polling (cherokee_thread_t *thd, cherokee_connection_t *conn)
{
	del_connection (thd, conn);
	add_connection_polling (thd, conn);

	return ret_ok;
}


static ret_t 
move_connection_to_active (cherokee_thread_t *thd, cherokee_connection_t *conn)
{
	del_connection_polling (thd, conn);
	add_connection (thd, conn);

	return ret_ok;
}


static ret_t 
reactive_conn_from_polling (cherokee_thread_t *thd, cherokee_connection_t *conn)
{	
	ret_t              ret;
	cherokee_socket_t *socket = &conn->socket;
	cherokee_boolean_t del    = true;

	TRACE (ENTRIES",polling", "conn=%p(fd=%d)\n", conn, SOCKET_FD(socket));

	/* Set the connection file descriptor and remove the old one
	 */
	if (conn->polling_multiple) 
		del = check_removal_multiple_fd (thd, conn->polling_fd);

	if (del) {
		ret = cherokee_fdpoll_del (thd->fdpoll, conn->polling_fd);
		if (ret != ret_ok)
			SHOULDNT_HAPPEN;
	}

	ret = cherokee_fdpoll_add (thd->fdpoll, socket->socket, socket->status);
	if (ret != ret_ok) {
		return ret_error;
	}

	/* Remove the polling fd from the connection
	 */
	conn->polling_fd       = -1;
	conn->polling_multiple = false;
	conn->polling_mode     = FDPOLL_MODE_NONE;

	BIT_SET (conn->options, conn_op_was_polling);

	return move_connection_to_active (thd, conn);
}


ret_t 
cherokee_thread_deactive_to_polling (cherokee_thread_t     *thd, 
				     cherokee_connection_t *conn, 
				     int                    fd, 
				     int                    rw, 
				     char                   multiple)
{	
	ret_t               ret;
	cherokee_boolean_t  add_fd = true;
	cherokee_socket_t  *socket = &conn->socket;

	TRACE (ENTRIES",polling", "conn=%p(fd=%d) (fd=%d, rw=%d)\n", 
	       conn, SOCKET_FD(socket), fd, rw);

	/* Check for fds added more than once
	 */
	if (multiple) 
		add_fd = check_addition_multiple_fd (thd, fd);

	/* Remove the connection file descriptor and add the new one
	 */
	ret = cherokee_fdpoll_del (thd->fdpoll, SOCKET_FD(socket));
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	if (add_fd) {
		ret = cherokee_fdpoll_add (thd->fdpoll, fd, rw);
		if (unlikely (ret != ret_ok)) {
			return ret_error;
		}
	}

	/* Set the information in the connection
	 */
	conn->polling_fd       = fd;
	conn->polling_mode     = rw;
	conn->polling_multiple = multiple;

	return move_connection_to_polling (thd, conn);
}


ret_t 
cherokee_thread_retire_active_connection (cherokee_thread_t *thd, cherokee_connection_t *conn)
{
	ret_t ret;

	ret = cherokee_fdpoll_del (thd->fdpoll, SOCKET_FD(&conn->socket));
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	del_connection (thd, conn);

	return ret_ok;
}


ret_t 
cherokee_thread_inject_active_connection (cherokee_thread_t *thd, cherokee_connection_t *conn)
{
	ret_t ret;

	ret = cherokee_fdpoll_add (thd->fdpoll, SOCKET_FD(&conn->socket), FDPOLL_MODE_WRITE);
	if (ret != ret_ok) {
		return ret_error;
	}

	add_connection (thd, conn);
	return ret_ok;
}
