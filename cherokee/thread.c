/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2010 Alvaro Lopez Ortega
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
#include "bogotime.h"
#include "limiter.h"


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
static NORETURN void *
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
	while (likely (thread->exit == false)) {
		cherokee_thread_step_MULTI_THREAD (thread, false);
	}

	thread->ended = true;
	pthread_detach (thread->thread);
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
	if (thd->ended)
		return ret_ok;

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
	n->ended               = false;
	n->is_full             = false;

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

	/* Thread Local Storage
	 */
	CHEROKEE_THREAD_PROP_SET (thread_error_writer_ptr, NULL);

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

	/* Traffic shaping
	 */
	cherokee_limiter_init (&n->limiter);

	/* The thread must adquire this mutex before
	 * process its connections
	 */
	CHEROKEE_MUTEX_INIT (&n->ownership, CHEROKEE_MUTEX_FAST);

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
conn_set_mode (cherokee_thread_t        *thd,
	       cherokee_connection_t    *conn,
	       cherokee_socket_status_t  s)
{
	if (conn->socket.status == s) {
		TRACE (ENTRIES, "Connection already in mode = %s\n", (s == socket_reading)? "reading" : "writing");
		return;
	}

	TRACE (ENTRIES, "Connection mode = %s\n", (s == socket_reading)? "reading" : "writing");

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
	cherokee_connection_update_vhost_traffic (conn);
	cherokee_connection_log (conn);

	/* Close & clean the socket and clean up the connection object
	 */
	cherokee_connection_clean_close (conn);

	if (thread->conns_num > 0) {
		thread->conns_num--;
	}

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
close_active_connection (cherokee_thread_t *thread, cherokee_connection_t *conn)
{
	ret_t ret;

	/* Delete from file descriptors poll
	 */
	ret = cherokee_fdpoll_del (thread->fdpoll, SOCKET_FD(&conn->socket));
	if (ret != ret_ok) {
		LOG_ERROR (CHEROKEE_ERROR_THREAD_RM_FD_POLL, SOCKET_FD(&conn->socket));
	}

	/* Remove from active connections list
	 */
	del_connection (thread, conn);

	/* Finally, purge connection
	 */
	purge_connection (thread, conn);
}


static void
maybe_purge_closed_connection (cherokee_thread_t *thread, cherokee_connection_t *conn)
{
	/* CONNECTION CLOSE: If it isn't a keep-alive connection, it
	 * should try to perform a lingering close (there is no need
	 * to disable TCP cork before shutdown or before a close).
	 * Logging is performed after the lingering close.
	 */
	if (conn->keepalive <= 0) {
		conn->phase = phase_shutdown;
		return;
	}

	conn->keepalive--;

	/* Log
	 */
	cherokee_connection_update_vhost_traffic (conn);
	cherokee_connection_log (conn);

	/* There might be data in the kernel buffer. Flush it before
	 * the connection is reused for the next keep-alive request.
	 * If not flushed, the data would remain on the buffer until
	 * the timeout is reached (with a huge performance penalty).
	 */
	cherokee_socket_flush (&conn->socket);

	/* Clean the connection
	 */
	cherokee_connection_clean (conn);
	conn_set_mode (thread, conn, socket_reading);

	/* Update the timeout value
	 */
	conn->timeout = cherokee_bogonow_now + conn->timeout_lapse;
}


static void
send_hardcoded_error (cherokee_socket_t *sock,
		      const char        *error,
		      cherokee_buffer_t *tmp)
{
	ret_t              ret;
	size_t             write;
	cherokee_boolean_t done  = false;

	cherokee_buffer_clean (tmp);
	cherokee_buffer_add_va (
		tmp,
		"HTTP/1.0 %s" CRLF_CRLF					\
		"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" CRLF \
		"<html><head><title>%s</title></head>" CRLF		\
		"<body><h1>%s</h1></body></html>",
		error, error, error);

	do {
		write = 0;

		ret = cherokee_socket_bufwrite (sock, tmp, &write);
		switch (ret) {
		case ret_ok:
			if (write > 0) {
				cherokee_buffer_move_to_begin (tmp, write);
			}
		default:
			done = true;
		}

		if (cherokee_buffer_is_empty (tmp))
			done = true;
	} while (!done);

}


static ret_t
process_polling_connections (cherokee_thread_t *thd)
{
	int                    re;
	cherokee_list_t       *tmp, *i;
	cherokee_connection_t *conn;

	list_for_each_safe (i, tmp, LIST(&thd->polling_list)) {
		conn = CONN(i);

		/* Thread's error logger
		 */
		if (CONN_VSRV(conn) &&
		    CONN_VSRV(conn)->error_writer)
		{
			CHEROKEE_THREAD_PROP_SET (thread_error_writer_ptr,
						  CONN_VSRV(conn)->error_writer);
		}

		/* Has it been too much without any work?
		 */
		if (conn->timeout < cherokee_bogonow_now) {
			TRACE (ENTRIES",polling,timeout",
			       "thread (%p) processing polling conn (%p, %s): Time out\n",
			       thd, conn, cherokee_connection_get_phase_str (conn));

			/* Information collection
			 */
			if (THREAD_SRV(thd)->collector != NULL) {
				cherokee_collector_log_timeout (THREAD_SRV(thd)->collector);
			}

			/* Close it
			 */
			if (conn->phase <= phase_add_headers) {
				send_hardcoded_error (&conn->socket,
						      http_gateway_timeout_string,
						      THREAD_TMP_BUF1(thd));
			}

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

		/* Thread's error logger
		 */
		if (CONN_VSRV(conn) &&
		    CONN_VSRV(conn)->error_writer)
		{
			CHEROKEE_THREAD_PROP_SET (thread_error_writer_ptr,
						  CONN_VSRV(conn)->error_writer);
		}

		/* Has the connection been too much time w/o any work
		 */
		if (conn->timeout < cherokee_bogonow_now) {
			TRACE (ENTRIES",polling,timeout",
			       "thread (%p) processing active conn (%p, %s): Time out\n",
			       thd, conn, cherokee_connection_get_phase_str (conn));

			/* Information collection
			 */
			if (THREAD_SRV(thd)->collector != NULL) {
				cherokee_collector_log_timeout (THREAD_SRV(thd)->collector);
			}

			conns_freed++;
			close_active_connection (thd, conn);
			continue;
		}

		/* Update the connection timeout
		 */
		if ((conn->phase != phase_reading_header) &&
		    (conn->phase != phase_reading_post) &&
		    (conn->phase != phase_lingering))
		{
			cherokee_connection_update_timeout (conn);
		}

		/* Maybe update traffic counters
		 */
		if ((CONN_VSRV(conn)->collector) &&
		    (conn->traffic_next < cherokee_bogonow_now) &&
		    ((conn->rx_partial != 0) || (conn->tx_partial != 0)))
		{
			cherokee_connection_update_vhost_traffic (conn);
		}

		/* Traffic shaping limiter
		 */
		if (conn->limit_blocked_until > 0) {
			cherokee_thread_retire_active_connection (thd, conn);
			cherokee_limiter_add_conn (&thd->limiter, conn);
			continue;
		}

		/* Check if the connection is active
		 */
		if (conn->options & conn_op_was_polling) {
			BIT_UNSET (conn->options, conn_op_was_polling);
		}
		else if ((conn->phase != phase_shutdown) &&
			 (conn->phase != phase_lingering) &&
			 (conn->phase != phase_reading_header || conn->incoming_header.len <= 0) &&
			 (conn->phase != phase_reading_post || conn->post.send.buffer.len <= 0))
		{
			re = cherokee_fdpoll_check (thd->fdpoll,
						    SOCKET_FD(&conn->socket),
						    conn->socket.status);
			switch (re) {
			case -1:
				conns_freed++;
				close_active_connection (thd, conn);
				continue;
			case 0:
				if (! cherokee_socket_pending_read (&conn->socket))
					continue;
			}
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
				goto shutdown;

			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				goto shutdown;
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
				goto shutdown;

			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				goto shutdown;
			}
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
					goto shutdown;
				default:
					RET_UNKNOWN(ret);
					conns_freed++;
					goto shutdown;
				}
			}

			/* Read from the client
			 */
			ret = cherokee_connection_recv (conn,
							&conn->incoming_header,
							DEFAULT_RECV_SIZE, &len);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eagain:
				continue;
			case ret_eof:
			case ret_error:
				conns_freed++;
				goto shutdown;
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				goto shutdown;
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
				goto shutdown;
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				goto shutdown;
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
				continue;
			}

			/* Thread's error logger
			 */
			if (CONN_VSRV(conn) &&
			    CONN_VSRV(conn)->error_writer)
			{
				CHEROKEE_THREAD_PROP_SET (thread_error_writer_ptr,
							  CONN_VSRV(conn)->error_writer);
			}

			/* Update timeout of the Keep-alive connections carried over..
			 * The previous timeout was set to allow them to linger open
			 * for a while. The new one is set to allow the server to serve
			 * the new request.
			 */
			if ((conn->keepalive > 0) &&
			    (conn->keepalive < CONN_SRV(conn)->keepalive_max))
			{
				cherokee_connection_update_timeout (conn);
			}

			/* Information collection
			 */
			if (THREAD_SRV(thd)->collector != NULL) {
				cherokee_collector_log_request (THREAD_SRV(thd)->collector);
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

			/* Is it already an error response?
			 */
			if (http_type_300(conn->error_code) ||
			    http_type_400(conn->error_code) ||
			    http_type_500(conn->error_code))
			{
				cherokee_connection_setup_error_handler (conn);
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

			/* Local directory
			 */
			if (cherokee_buffer_is_empty (&conn->local_directory)) {
				if (is_userdir)
					ret = cherokee_connection_build_local_directory_userdir (conn, CONN_VSRV(conn));
				else
					ret = cherokee_connection_build_local_directory (conn, CONN_VSRV(conn));
			}

			/* Check against the rule list
			 */
			ret = cherokee_rule_list_match (rules, conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				continue;
			}

			/* Local directory
			 */
			cherokee_connection_set_custom_droot (conn, &entry);

			/* Set the logger of the connection
			 */
			if (entry.no_log != true) {
				conn->logger_ref = CONN_VSRV(conn)->logger;
			}

			/* Check of the HTTP method is supported by the handler
			 */
			ret = cherokee_connection_check_http_method (conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				continue;
			}

			/* Check Only-Secure connections
			 */
			ret = cherokee_connection_check_only_secure (conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				continue;
			}

			/* Check for IP validation
			 */
			ret = cherokee_connection_check_ip_validation (conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				continue;
			}

			/* Check for authentication
			 */
			ret = cherokee_connection_check_authentication (conn, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				continue;
			}

			/* Update the keep-alive property
			 */
			cherokee_connection_set_keepalive (conn);

			/* Traffic Shaping
			 */
			cherokee_connection_set_rate (conn, &entry);

			/* Custom timeout
			 */
			if (! NULLI_IS_NULL(entry.timeout_lapse)) {
				conn->timeout_lapse  = entry.timeout_lapse;
				conn->timeout_header = entry.timeout_header;
			}

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
				continue;
			}

			/* Turn chunked encoding on, if possible
			*/
			cherokee_connection_set_chunked_encoding (conn);

			/* Instance an encoder if needed
			*/
			ret = cherokee_connection_create_encoder (conn, entry.encoders);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				continue;
			}

			/* Parse the rest of headers
			 */
			ret = cherokee_connection_parse_range (conn);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
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
					LOG_ERROR (CHEROKEE_ERROR_THREAD_HANDLER_RET,
						   ret, MODULE(conn->handler)->info->name);
				else
					RET_UNKNOWN(ret);
				break;
			}

			/* If it is an error, and the connection has not a handler to manage
			 * this error, the handler has to be changed by an error_handler.
			 */
			if (conn->handler == NULL) {
				conns_freed++;
				goto shutdown;
			}

 			if (http_type_300(conn->error_code) ||
			    http_type_400(conn->error_code) ||
			    http_type_500(conn->error_code))
			{
				if (HANDLER_SUPPORTS (conn->handler, hsupport_error)) {
					ret = cherokee_connection_clean_error_headers (conn);
					if (unlikely (ret != ret_ok)) {
						conns_freed++;
						goto shutdown;
					}
				} else {
					/* Try to setup an error handler
					 */
					ret = cherokee_connection_setup_error_handler (conn);
					if ((ret != ret_ok) &&
					    (ret != ret_eagain))
					{
						/* Critical error: It couldn't instance the handler
						 */
						conns_freed++;
						goto shutdown;
					}
					continue;
				}
			}

			/* Figure next state
			 */
			if (! http_method_with_input (conn->header.method)) {
				conn->phase = phase_add_headers;
				goto add_headers;
			}

			/* Register with the POST tracker
			 */
			if ((srv->post_track) && (conn->post.has_info)) {
				srv->post_track->func_register (srv->post_track, conn);
			}

			conn->phase = phase_reading_post;

		case phase_reading_post:
			/* Read/Send the POST info
			 */
			ret = cherokee_connection_read_post (conn);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eagain:
				/* Blocking on socket read */
				conn_set_mode (thd, conn, socket_reading);
				continue;
			case ret_deny:
				/* Blocking on back-end write */
				continue;
			case ret_eof:
			case ret_error:
				conn->error_code = http_internal_error;
				cherokee_connection_setup_error_handler (conn);
				continue;
			default:
				RET_UNKNOWN(ret);
			}

			/* Turn the connection in write mode
			 */
			conn_set_mode (thd, conn, socket_writing);
			conn->phase = phase_add_headers;

		case phase_add_headers:
		add_headers:

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
				conn->error_code = http_internal_error;
				cherokee_connection_setup_error_handler (conn);
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
				goto shutdown;

			default:
				RET_UNKNOWN(ret);
			}

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
					goto shutdown;

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
					goto shutdown;
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
					goto shutdown;
				}
				break;

			case ret_ok_and_sent:
				break;

			case ret_eof:
				maybe_purge_closed_connection (thd, conn);
				continue;

			case ret_error:
				goto shutdown;

			default:
				RET_UNKNOWN(ret);
				goto shutdown;
			}
			break;

		case phase_shutdown:
		shutdown:
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
				close_active_connection (thd, conn);
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
				close_active_connection (thd, conn);
				continue;
			default:
				RET_UNKNOWN(ret);
				conns_freed++;
				close_active_connection (thd, conn);
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

	cherokee_limiter_mrproper (&thd->limiter);

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
thread_full_handler (cherokee_thread_t *thd,
		     cherokee_bind_t   *bind)
{
	ret_t              ret;
	cherokee_list_t   *i;
	cherokee_socket_t  sock;
	cherokee_server_t *srv   = THREAD_SRV(thd);
	cherokee_buffer_t *tmp   = THREAD_TMP_BUF1(thd);

	/* Short path: nothing to accept
	 */
	if (cherokee_fdpoll_check (thd->fdpoll, S_SOCKET_FD(bind->socket), FDPOLL_MODE_READ) <= 0) {
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
		ret = cherokee_socket_accept (&sock, &bind->socket);
	} while (ret == ret_deny);

	if (ret != ret_ok)
		goto out;

	LOG_WARNING_S (CHEROKEE_ERROR_THREAD_OUT_OF_FDS);

	/* Read the request
	 */
#if 0
	do {
		ret = cherokee_socket_read (&sock, tmp->buf, tmp->size, &read);
	} while (ret == ret_eagain);
#endif

	/* Write the error response
	 */
	send_hardcoded_error (&sock, http_service_unavailable_string, tmp);

out:
	cherokee_socket_close (&sock);
	cherokee_socket_mrproper (&sock);
}


static ret_t
accept_new_connection (cherokee_thread_t *thd,
		       cherokee_bind_t   *bind)
{
	int                    re;
	ret_t                  ret;
	int                    new_fd;
	cherokee_sockaddr_t    new_sa;
	cherokee_connection_t *new_conn;

	/* Check whether there are connections waiting
	 */
	re = cherokee_fdpoll_check (thd->fdpoll, S_SOCKET_FD(bind->socket), FDPOLL_MODE_READ);
	if (re <= 0) {
		return ret_deny;
	}

	/* Try to get a new connection
	 */
	ret = cherokee_socket_accept_fd (&bind->socket, &new_fd, &new_sa);
	if (ret != ret_ok) {
		return ret_deny;
	}

	/* Information collection
	 */
	if (THREAD_SRV(thd)->collector != NULL) {
		cherokee_collector_log_accept (THREAD_SRV(thd)->collector);
	}

	/* We got the new socket, now set it up in a new connection object
	 */
	ret = cherokee_thread_get_new_connection (thd, &new_conn);
	if (unlikely(ret < ret_ok)) {
		LOG_ERROR_S (CHEROKEE_ERROR_THREAD_GET_CONN_OBJ);
		cherokee_fd_close (new_fd);
		return ret_deny;
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
		LOG_ERROR_S (CHEROKEE_ERROR_THREAD_SET_SOCKADDR);
		goto error;
	}

	/* TLS support, set initial connection phase.
	 */
	if (bind->socket.is_tls == TLS) {
		new_conn->phase = phase_tls_handshake;
	}

	/* Set the reference to the port
	 */
	new_conn->bind = bind;

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
	return ret_ok;

error:
	TRACE (ENTRIES, "error accepting connection fd %d from port %d\n",
	       new_fd, bind->port);

	/* Close new socket and reset its socket fd to default value.
	 */
	cherokee_fd_close (new_fd);
	S_SOCKET_FD(new_conn->socket) = -1;

	/* Don't waste / reuse this new_conn object.
	 */
	connection_reuse_or_free (thd, new_conn);

	/* Release the thread ownership
	 */
	CHEROKEE_MUTEX_UNLOCK (&thd->ownership);
	return ret_deny;
}


static ret_t
should_accept_more (cherokee_thread_t *thd,
		    cherokee_bind_t   *bind,
		    ret_t              prev_ret)
{
	/* If it is full, do not accept more!
	 */
	if (unlikely (thd->conns_num >= thd->conns_max))
		return ret_deny;

	if (unlikely ((THREAD_SRV(thd)->wanna_reinit) ||
		      (THREAD_SRV(thd)->wanna_exit)))
		return ret_deny;
#if 0
	if (unlikely (cherokee_fdpoll_is_full(thd->fdpoll))) {
		return ret_deny;
	}
#endif

	return cherokee_bind_accept_more (bind, prev_ret);
}


ret_t
cherokee_thread_step_SINGLE_THREAD (cherokee_thread_t *thd)
{
	ret_t              ret;
	cherokee_boolean_t accepting;
	cherokee_list_t   *i;
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

	/* May have to reactive connections
	 */
	cherokee_limiter_reactive (&thd->limiter, thd);

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

	/* Reactive sleeping connections
	 */
	fdwatch_msecs = cherokee_limiter_get_time_limit (&thd->limiter,
							 fdwatch_msecs);

	/* Graceful restart
	 */
	if (srv->wanna_reinit) {
		if ((thd->active_list_num == 0) &&
		    (thd->polling_list_num == 0))
		{
			thd->exit = true;
			return ret_eof;
		}

		cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
		goto out;
	}

	/* Inspect the file descriptors
	 */
	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	thread_update_bogo_now (thd);

	/* Accept new connections, if possible
	 */
	accepting = (thd->conns_num < thd->conns_max);

	list_for_each (i, &srv->listeners) {
		if (! accepting) {
			thread_full_handler (thd, BIND(i));
			continue;
		}

		do {
			ret = accept_new_connection (thd, BIND(i));
		} while (should_accept_more (thd, BIND(i), ret) == ret_ok);
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

static void
watch_accept_MULTI_THREAD (cherokee_thread_t  *thd,
			   cherokee_boolean_t  block,
			   int                 fdwatch_msecs)
{
	ret_t               ret;
	int                 unlocked;
	cherokee_bind_t    *bind;
	cherokee_list_t    *i;
	cherokee_boolean_t  yield      = false;
 	cherokee_server_t  *srv        = THREAD_SRV(thd);

	/* Lock
	 */
	if (block) {
		CHEROKEE_MUTEX_LOCK (&srv->listeners_mutex);
	} else {
		unlocked = CHEROKEE_MUTEX_TRY_LOCK (&srv->listeners_mutex);
		if (unlocked) {
			cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
			return;
		}
	}

	/* Shortcut: don't waste time on watch() */
	if (unlikely ((srv->wanna_exit) ||
		      ((srv->wanna_reinit) &&
		       (thd->active_list_num  == 0) &&
		       (thd->polling_list_num == 0))))
	{
		goto out;
	}

	/* Locked; Add port file descriptors
	 */
	list_for_each (i, &srv->listeners) {
		ret = cherokee_fdpoll_add (thd->fdpoll,
					   S_SOCKET_FD(BIND(i)->socket),
					   FDPOLL_MODE_READ);
		if (unlikely (ret < ret_ok)) {
			ret = ret_error;
			goto out;
		}
	}

	/* Check file descriptors
	 */
	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	thread_update_bogo_now (thd);

	/* Accept new connections
	 */
	list_for_each (i, &srv->listeners) {
		bind = BIND(i);

		/* Is it full?
		 */
		if (unlikely (thd->conns_num >= thd->conns_max)) {
			if (thd->is_full) {
				thread_full_handler (thd, bind);
				thd->is_full = false;
			} else {
				thd->is_full = true;
			}

			yield = true;
			break;
		} else {
			thd->is_full = false;
		}

		/* Accept new connections
		 */
		do {
			ret = accept_new_connection (thd, bind);
		} while (should_accept_more (thd, bind, ret) == ret_ok);
	}

	/* Release the port file descriptors
	 */
	list_for_each (i, &srv->listeners) {
		ret = cherokee_fdpoll_del (thd->fdpoll, S_SOCKET_FD(BIND(i)->socket));
		if (ret != ret_ok) {
			SHOULDNT_HAPPEN;
		}
	}

out:
	/* Unlock
	 */
	CHEROKEE_MUTEX_UNLOCK (&srv->listeners_mutex);

	if (yield) {
		CHEROKEE_THREAD_YIELD;
	}
}


ret_t
cherokee_thread_step_MULTI_THREAD (cherokee_thread_t  *thd,
				   cherokee_boolean_t  dont_block)
{
	ret_t              ret;
	cherokee_boolean_t time_updated;
	cherokee_boolean_t can_block     = false;
	cherokee_server_t *srv           = THREAD_SRV(thd);
	int                fdwatch_msecs = srv->fdwatch_msecs;

	/* Try to update bogo_now
	 */
	ret = cherokee_bogotime_try_update();
	time_updated = (ret == ret_ok);

	/* May have to reactive connections
	 */
	cherokee_limiter_reactive (&thd->limiter, thd);

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

	/* Reactive sleeping connections
	 */
	fdwatch_msecs = cherokee_limiter_get_time_limit (&thd->limiter,
							 fdwatch_msecs);

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

# if 0
	if (unlikely (cherokee_fdpoll_is_full (thd->fdpoll))) {
		goto out;
	}
# endif

	/* Watch fds, and accept new connections
	 */
	can_block = ((dont_block == false) &&
		     (thd->exit == false) &&
		     (thd->active_list_num == 0) &&
		     (thd->polling_list_num == 0) &&
		     (thd->limiter.conns_num == 0));

	watch_accept_MULTI_THREAD (thd, can_block, fdwatch_msecs);

out:
	if ((can_block) ||
	    (time_updated == false))
	{
		thread_update_bogo_now (thd);
	}

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

	new_connection->traffic_next = cherokee_bogonow_now + DEFAULT_TRAFFIC_UPDATE;

	/* Set the default server timeout
	 */
	new_connection->timeout        = cherokee_bogonow_now + server->timeout;
	new_connection->timeout_lapse  = server->timeout;
	new_connection->timeout_header = &server->timeout_header;

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
