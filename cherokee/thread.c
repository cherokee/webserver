/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
#include "request.h"
#include "request-protected.h"
#include "handler_error.h"
#include "header.h"
#include "header-protected.h"
#include "util.h"
#include "bogotime.h"
#include "limiter.h"
#include "flcache.h"


#define DEBUG_BUFFER(b)  fprintf(stderr, "%s:%d len=%d crc=%d\n", __FILE__, __LINE__, b->len, cherokee_buffer_crc32(b))
#define ENTRIES "core,thread"

static ret_t reactive_conn_from_polling (cherokee_thread_t *thd, cherokee_request_t *conn);
static ret_t move_connection_to_polling (cherokee_thread_t *thd, cherokee_request_t *conn);


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
	INIT_LIST_HEAD (&n->base);
	INIT_LIST_HEAD (&n->active_list);
	INIT_LIST_HEAD (&n->polling_list);

	n->exit                = false;
	n->ended               = false;
	n->is_full             = false;

	n->server              = server;
	n->thread_type         = type;

	n->conns_num           = 0;
	n->conns_max           = conns_max;
	n->conns_keepalive_max = keepalive_max;

	n->fastcgi_servers     = NULL;
	n->fastcgi_free_func   = NULL;

	/* Reuse */
	INIT_LIST_HEAD (&n->reuse.reqs);
	INIT_LIST_HEAD (&n->reuse.conns);

	n->reuse.reqs_num  = 0;
	n->reuse.conns_num = 0;

	/* Thread Local Storage
	 */
	CHEROKEE_THREAD_PROP_SET (thread_error_writer_ptr, NULL);
	CHEROKEE_THREAD_PROP_SET (thread_connection_ptr,   NULL);

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
		int            re;
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
		re = pthread_create (&n->thread, &attr, thread_routine, n);
		if (unlikely (re != 0)) {
			LOG_ERRNO (re, cherokee_err_error, CHEROKEE_ERROR_THREAD_CREATE, re);

			pthread_attr_destroy (&attr);
			cherokee_thread_free (n);
			return ret_error;
		}

		pthread_attr_destroy (&attr);
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
add_connection (cherokee_thread_t *thd, cherokee_request_t *req)
{
	cherokee_list_add_tail (LIST(req), &thd->active_list);
}

static void
add_connection_polling (cherokee_thread_t *thd, cherokee_request_t *req)
{
	cherokee_list_add_tail (LIST(req), &thd->polling_list);
}

static void
del_connection (cherokee_thread_t *thd, cherokee_request_t *req)
{
	UNUSED (thd);
	cherokee_list_del (LIST(req));
}

static void
del_connection_polling (cherokee_thread_t *thd, cherokee_request_t *req)
{
	UNUSED (thd);
	cherokee_list_del (LIST(req));
}


ret_t
cherokee_thread_get_new_connection (cherokee_thread_t      *thd,
				    cherokee_connection_t **conn)
{
	/* New obj */
	if (thread->reuse.conns_num <= 0) {
		return cherokee_connection_new (conn);
	}

	/* Reuse obj */
	*conn = CONN(thd->reuse.conns.prev);
	cherokee_list_del (LIST(*conn));
	INIT_LIST_HEAD (LIST(*conn));
	thread->reuse.conns_num--;

	return ret_ok;
}

ret_t
cherokee_thread_recycle_connection (cherokee_thread_t     *thd,
				    cherokee_connection_t *conn)
{
	/* Disable keepalive in the connection
	 */
	conn->keepalive = 0;

	/* Check the max connection reuse number
	 */
	if (thread->reuse.conns_num >= THREAD_SRV(thread)->conns_reuse_max) {
		return cherokee_connection_free (conn);
	}

	/* Add it to the reusable connection list
	 */
	cherokee_list_add (LIST(conn), &thread->reuse.conns);
	thread->reuse.conns_num++;

	return ret_ok;
}


ret_t
cherokee_thread_get_new_request (cherokee_thread_t   *thd,
				 cherokee_request_t **req)
{
	/* New obj */
	if (thread->reuse.reqs_num <= 0) {
		return cherokee_request_new (req);
	}

	/* Reuse obj */
	*req = REQ(thd->reuse.reqs.prev);
	cherokee_list_del (LIST(*req));
	INIT_LIST_HEAD (LIST(*req));
	thread->reuse.reqs_num--;

	return ret_ok;
}


ret_t
cherokee_thread_recycle_request (cherokee_thread_t     *thd,
				 cherokee_connection_t *req)
{
	/* Check the max connection reuse number
	 */
	if (thread->reuse.reqs_num >= THREAD_SRV(thread)->conns_reuse_max) {
		return cherokee_request_free (req);
	}

	/* Add it to the reusable connection list
	 */
	cherokee_list_add (LIST(req), &thread->reuse.reqs);
	thread->reuse.reqs_num++;

	return ret_ok;
}


static void
purge_connection (cherokee_thread_t *thread, cherokee_request_t *req)
{
	/* It maybe have a delayed log
	 */
	cherokee_request_update_vhost_traffic (req);
	cherokee_request_log (req);

	/* Close & clean the socket and clean up the connection object
	 */
	cherokee_request_clean_close (req);

	if (thread->conns_num > 0) {
		thread->conns_num--;
	}

	/* Add it to the reusable list
	 */
	cherokee_thread_recycle_connection (thread, conn);
}


static void
purge_closed_polling_connection (cherokee_thread_t *thread, cherokee_request_t *req)
{
	ret_t ret;

	/* Delete from file descriptors poll
	 */
	ret = cherokee_fdpoll_del (thread->fdpoll, req->polling_aim.fd);
	if (ret != ret_ok) {
		SHOULDNT_HAPPEN;
	}

	/* Remove from the polling list
	 */
	del_connection_polling (thread, req);

	/* The connection hasn't the main fd in the file descriptor poll
	 * so, we just have to remove the connection.
	 */
	purge_connection (thread, req);
}


static void
close_active_connection (cherokee_thread_t  *thread,
			 cherokee_request_t *req,
			 cherokee_boolean_t  reset)
{
	/* Force to send a RST
	 */
	if (reset) {
		cherokee_socket_reset (&req->socket);
	}

	/* Remove from active connections list
	 */
	del_connection (thread, req);

	/* Finally, purge connection
	 */
	purge_connection (thread, req);
}


static void
finalize_request (cherokee_thread_t  *thread,
		  cherokee_request_t *req)
{
	/* CONNECTION CLOSE: If it isn't a keep-alive connection, it
	 * should try to perform a lingering close (there is no need
	 * to disable TCP cork before shutdown or before a close).
	 * Logging is performed after the lingering close.
	 */
	if (req->keepalive <= 1) {
		req->phase = phase_shutdown;
		return;
	}

	req->keepalive--;

	/* Log
	 */
	cherokee_request_update_vhost_traffic (req);
	cherokee_request_log (req);

	/* There might be data in the kernel buffer. Flush it before
	 * the connection is reused for the next keep-alive request.
	 * If not flushed, the data would remain on the buffer until
	 * the timeout is reached (with a huge performance penalty).
	 */
	cherokee_socket_flush (&req->socket);

	/* Clean the connection
	 */
	cherokee_request_clean (req, true);

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
	cherokee_buffer_add_va (tmp,
				"HTTP/1.0 %s" CRLF_CRLF			\
				"<!DOCTYPE html>" CRLF			\
				"<html><head><title>%s</title></head>" CRLF \
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


static void
process_polling_connections (cherokee_thread_t *thd)
{
	int                    re;
	ret_t                  ret;
	cherokee_connection_t *conn;
	cherokee_list_t       *tmp, *i;

	list_for_each_safe (i, tmp, LIST(&thd->polling_list)) {
		conn = CONN(i);

		/* Thread's properties
		 */
		if (REQ_VSRV(req)) {
			/* Current connection
			 */
			CHEROKEE_THREAD_PROP_SET (thread_connection_ptr, req);

			/* Error writer
			 */
			if (REQ_VSRV(req)->error_writer) {
				CHEROKEE_THREAD_PROP_SET (thread_error_writer_ptr,
							  REQ_VSRV(req)->error_writer);
			}
		}

		/* Has the connection expired?
		 */
		if (conn->timeout < cherokee_bogonow_now) {
			TRACE (ENTRIES",polling,timeout",
			       "processing polling conn (%p, %s): Time out\n",
			       conn, cherokee_request_get_phase_str (req));

			/* Information collection
			 */
			if (THREAD_SRV(thd)->collector != NULL) {
				cherokee_collector_log_timeout (THREAD_SRV(thd)->collector);
			}

			/* Most likely a 'Gateway Timeout'
			 */
			if ((req->phase >= phase_processing_header) ||
			    ((req->phase == phase_reading_header) && (req->incoming_header.len >= 1)))
			{
				/* Push a hardcoded error
				 */
				send_hardcoded_error (&req->socket,
						      http_gateway_timeout_string,
						      THREAD_TMP_BUF1(thd));

				/* Assign the error code. Even though it wasn't used
				 * before the handler::free function could check it.
				 */
				req->error_code = http_gateway_timeout;

				/* Purge the connection
				 */
				purge_closed_polling_connection (thd, req);
				continue;
			}

			/* Timed-out: Reactive the connection. The
			 * main loop will take care of closing it.
			 */
			ret = reactive_conn_from_polling (thd, req);
			if (unlikely (ret != ret_ok)) {
				purge_closed_polling_connection (thd, req);
				continue;
			}

			continue;
		}

		/* Is there information to be sent?
		 */
		if (req->buffer.len > 0) {
			ret = reactive_conn_from_polling (thd, req);
			if (unlikely (ret != ret_ok)) {
				purge_closed_polling_connection (thd, req);
				continue;
			}
			continue;
		}

		/* Check the "extra" file descriptor
		 */
		re = cherokee_fdpoll_check (thd->fdpoll, req->polling_aim.fd, req->polling_aim.mode);
		switch (re) {
		case -1:
			/* Error, move back the connection
			 */
			TRACE (ENTRIES",polling", "conn %p(fd=%d): status is Error (fd=%d)\n",
			       req, SOCKET_FD(&req->socket), req->polling_aim.fd);

			purge_closed_polling_connection (thd, req);
			continue;
		case 0:
			/* Nothing to do.. wait longer
			 */
			;
			continue;
		}

		/* Move from the 'polling' to the 'active' list:
		 */
		ret = reactive_conn_from_polling (thd, req);
		if (unlikely (ret != ret_ok)) {
			purge_closed_polling_connection (thd, req);
			continue;
		}
	}
}


static void
process_active_connections (cherokee_thread_t *thd)
{
	ret_t                      ret;
	off_t                      len;
	cherokee_list_t           *i, *tmp1;
	cherokee_list_t           *j, *tmp2;
	cherokee_request_t        *req       = NULL;
	cherokee_connection_t     *conn      = NULL;
	cherokee_virtual_server_t *vsrv      = NULL;
	cherokee_server_t         *srv       = SRV(thd->server);
	cherokee_socket_status_t   blocking;

#ifdef TRACE_ENABLED
	if (cherokee_trace_is_tracing()) {
		if (! cherokee_list_empty (&thd->active_list)) {
			TRACE (ENTRIES",active", "Active connections:%s", "\n");
		}

		list_for_each_safe (i, tmp, &thd->active_list) {
			req = REQ(i);

			TRACE (ENTRIES",active", "   \\- processing conn (%p), phase %d '%s', socket=%d\n",
			       req, req->phase, cherokee_request_get_phase_str (req), req->socket.socket);
		}
	}
#endif

	/* Process active connections
	 */
	list_for_each_safe (i, tmp1, LIST(&thd->active_list)) {
		conn = CONN(i);

		/* Thread property: Current connection
		 */
		CHEROKEE_THREAD_PROP_SET (thread_connection_ptr, conn);



		/* Has the connection been too much time w/o any work
		 */
		if (conn->timeout < cherokee_bogonow_now) {
			TRACE (ENTRIES",polling,timeout",
			       "processing active conn (%p, %s): Time out\n",
			       conn, cherokee_request_get_phase_str (conn));

			/* The lingering close timeout expired.
			 * Proceed to close the connection.
			 */
			if ((req->phase == phase_shutdown) ||
			    (req->phase == phase_lingering))
			{
				close_active_connection (thd, req, false);
				continue;
			}

				/* Information collection
				 */
				if (THREAD_SRV(thd)->collector != NULL) {
					cherokee_collector_log_timeout (THREAD_SRV(thd)->collector);
				}

				goto shutdown;






		list_for_each_safe (j, tmp2, LIST(conn->requests)) {
			req  = REQ(i);
			vsrv = REQ_VSRV(i);

			TRACE (ENTRIES, "processing req (c=%p,r=%p), phase %d '%s', socket=%d\n",
			       conn, req, req->phase, cherokee_request_get_phase_str (req), req->socket.socket);

			/* Thread properties: Current request
			 */
			CHEROKEE_THREAD_PROP_SET (thread_request_ptr, req);

			/* Error writer */
			if ((vsrv != NULL) && (vsrv->error_writer != NULL)) {
				CHEROKEE_THREAD_PROP_SET (thread_error_writer_ptr, vsrv->error_writer);
			}

			/* Has the connection been too much time w/o any work
			 */
			if (conn->timeout < cherokee_bogonow_now) {
				TRACE (ENTRIES",polling,timeout",
				       "processing active conn (%p, %s): Time out\n",
				       req, cherokee_request_get_phase_str (req));

				/* The lingering close timeout expired.
				 * Proceed to close the connection.
				 */
				if ((req->phase == phase_shutdown) ||
				    (req->phase == phase_lingering))
				{
					close_active_connection (thd, req, false);
					continue;
				}

				/* Information collection
				 */
				if (THREAD_SRV(thd)->collector != NULL) {
					cherokee_collector_log_timeout (THREAD_SRV(thd)->collector);
				}

				goto shutdown;
			}

			/* Update the connection timeout
			 */
			if ((req->phase != phase_tls_handshake) &&
			    (req->phase != phase_reading_header) &&
			    (req->phase != phase_reading_post) &&
			    (req->phase != phase_shutdown) &&
			    (req->phase != phase_lingering))
			{
				cherokee_request_update_timeout (req);
			}

			/* Maybe update traffic counters
			 */
			if ((vsrv->collector) &&
			    (req->traffic_next < cherokee_bogonow_now) &&
			    ((req->rx_partial != 0) || (req->tx_partial != 0)))
			{
				cherokee_request_update_vhost_traffic (req);
			}

			/* Traffic shaping limiter
			 */
			if (req->limit_blocked_until > 0) {
				cherokee_thread_retire_active_connection (thd, req);
				cherokee_limiter_add_conn (&thd->limiter, req);
				continue;
			}

			TRACE (ENTRIES, "conn on phase n=%d: %s\n",
			       req->phase, cherokee_request_get_phase_str (req));

			/* Phases
			 */
			switch (req->phase) {
			case phase_tls_handshake:
				blocking = socket_closed;

				ret = cherokee_socket_init_tls (&req->socket, vsrv, req, &blocking);
				switch (ret) {
				case ret_eagain:
					switch (blocking) {
					case socket_reading:
						break;

					case socket_writing:
						break;

					default:
						break;
					}

					continue;

				case ret_ok:
					TRACE(ENTRIES, "Handshake %s\n", "finished");

					/* Set mode and update timeout
					 */
					cherokee_request_update_timeout (req);

					req->phase = phase_reading_header;
					break;

				case ret_eof:
				case ret_error:
					goto shutdown;

				default:
					RET_UNKNOWN(ret);
					goto shutdown;
				}
				break;

			case phase_reading_header:
				/* Maybe the buffer has a request (previous pipelined)
				 */
				if (! cherokee_buffer_is_empty (&req->incoming_header))
				{
					ret = cherokee_header_has_header (&req->header,
									  &req->incoming_header,
									  req->incoming_header.len);
					switch (ret) {
					case ret_ok:
						goto phase_reading_header_EXIT;
					case ret_not_found:
						break;
					case ret_error:
						goto shutdown;
					default:
						RET_UNKNOWN(ret);
						goto shutdown;
					}
				}

				/* Read from the client
				 */
				ret = cherokee_request_recv (req,
							     &req->incoming_header,
							     DEFAULT_RECV_SIZE, &len);
				switch (ret) {
				case ret_ok:
					break;
				case ret_eagain:
					cherokee_thread_deactive_to_polling (thd, req);
					continue;
				case ret_eof:
				case ret_error:
					goto shutdown;
				default:
					RET_UNKNOWN(ret);
					goto shutdown;
				}

				/* Check security after read
				 */
				ret = cherokee_request_reading_check (req);
				if (ret != ret_ok) {
					req->keepalive      = 0;
					req->phase          = phase_setup_connection;
					req->header.version = http_version_11;
					continue;
				}

				/* May it already has the full header
				 */
				ret = cherokee_header_has_header (&req->header, &req->incoming_header, len+4);
				switch (ret) {
				case ret_ok:
					break;
				case ret_not_found:
					req->phase = phase_reading_header;
					continue;
				case ret_error:
					goto shutdown;
				default:
					RET_UNKNOWN(ret);
					goto shutdown;
				}

				/* fall down */

			phase_reading_header_EXIT:
				req->phase = phase_processing_header;

				/* fall down */

			case phase_processing_header:
				/* Get the request
				 */
				ret = cherokee_request_get_request (req);
				switch (ret) {
				case ret_ok:
					break;

				case ret_eagain:
					continue;

				default:
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Thread's error logger
				 */
				if (vsrv && vsrv->error_writer) {
					CHEROKEE_THREAD_PROP_SET (thread_error_writer_ptr, vsrv->error_writer);
				}

				/* Update timeout of the Keep-alive connections carried over..
				 * The previous timeout was set to allow them to linger open
				 * for a while. The new one is set to allow the server to serve
				 * the new request.
				 */
				if ((req->keepalive > 0) &&
				    (req->keepalive < REQ_SRV(req)->keepalive_max))
				{
					cherokee_request_update_timeout (req);
				}

				/* Information collection
				 */
				if (THREAD_SRV(thd)->collector != NULL) {
					cherokee_collector_log_request (THREAD_SRV(thd)->collector);
				}

				req->phase = phase_setup_connection;

				/* fall down */

			case phase_setup_connection: {
				cherokee_rule_list_t *rules;
				cherokee_boolean_t    is_userdir;

				/* HSTS support
				 */
				if ((req->socket.is_tls != TLS) &&
				    (vsrv->hsts.enabled))
				{
					cherokee_request_setup_hsts_handler (req);
					continue;
				}

				/* Is it already an error response?
				 */
				if (http_type_300 (req->error_code) ||
				    http_type_400 (req->error_code) ||
				    http_type_500 (req->error_code))
				{
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Front-line cache
				 */
				if ((vsrv->flcache) &&
				    (req->header.method == http_get))
				{
					TRACE (ENTRIES, "Front-line cache available: '%s'\n", vsrv->name.buf);

					ret = cherokee_flcache_req_get_cached (vsrv->flcache, req);
					if (ret == ret_ok) {
						/* Set Keepalive, Rate, and skip to add_headers
						 */
						cherokee_request_set_keepalive (req);
						cherokee_request_set_rate (req, &req->config_entry);

						req->phase = phase_add_headers;
						goto add_headers;
					}
				}

				TRACE (ENTRIES, "Setup connection begins: request=\"%s\"\n", req->request.buf);
				TRACE_REQ(req);

				cherokee_config_entry_ref_clean (&req->config_entry);

				/* Choose the virtual entries table
				 */
				is_userdir = ((vsrv->userdir.len > 0) && (req->userdir.len > 0));

				if (is_userdir) {
					rules = &vsrv->userdir_rules;
				} else {
					rules = &vsrv->rules;
				}

				/* Local directory
				 */
				if (cherokee_buffer_is_empty (&req->local_directory)) {
					if (is_userdir)
						ret = cherokee_request_build_local_directory_userdir (req, vsrv);
					else
						ret = cherokee_request_build_local_directory (req, vsrv);
				}

				/* Check against the rule list. It fills out ->config_entry, and
				 * req->auth_type
				 * req->expiration*
				 * conn->timeout_*
				 */
				ret = cherokee_rule_list_match (rules, req, &req->config_entry);
				if (unlikely (ret != ret_ok)) {
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Local directory
				 */
				cherokee_request_set_custom_droot (req, &req->config_entry);

				/* Set the logger of the connection
				 */
				if (req->config_entry.no_log != true) {
					req->logger_ref = vsrv->logger;
				}

				/* Check of the HTTP method is supported by the handler
				 */
				ret = cherokee_request_check_http_method (req, &req->config_entry);
				if (unlikely (ret != ret_ok)) {
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Check Only-Secure connections
				 */
				ret = cherokee_request_check_only_secure (req, &req->config_entry);
				if (unlikely (ret != ret_ok)) {
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Check for IP validation
				 */
				ret = cherokee_request_check_ip_validation (req, &req->config_entry);
				if (unlikely (ret != ret_ok)) {
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Check for authentication
				 */
				ret = cherokee_request_check_authentication (req, &req->config_entry);
				if (unlikely (ret != ret_ok)) {
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Update the keep-alive property
				 */
				cherokee_request_set_keepalive (req);

				/* Traffic Shaping
				 */
				cherokee_request_set_rate (req, &req->config_entry);

				/* Create the handler
				 */
				ret = cherokee_request_create_handler (req, &req->config_entry);
				switch (ret) {
				case ret_ok:
					break;
				case ret_eagain:
					cherokee_request_clean_for_respin (req);
					continue;
				case ret_eof:
					/* Connection drop */
					close_active_connection (thd, req, true);
					continue;
				default:
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Turn chunked encoding on, if possible
				 */
				cherokee_request_set_chunked_encoding (req);

				/* Instance an encoder if needed
				 */
				ret = cherokee_request_create_encoder (req, req->config_entry.encoders);
				if (unlikely (ret != ret_ok)) {
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Parse the rest of headers
				 */
				ret = cherokee_request_parse_range (req);
				if (unlikely (ret != ret_ok)) {
					cherokee_request_setup_error_handler (req);
					continue;
				}

				/* Front-line cache
				 */
				if ((vsrv->flcache != NULL) &&
				    (req->config_entry.flcache == true) &&
				    (cherokee_flcache_req_is_storable (vsrv->flcache, req) == ret_ok))
				{
					cherokee_flcache_req_set_store (vsrv->flcache, req);

					/* Update expiration
					 */
					if (req->flcache.mode == flcache_mode_in) {
						if (req->config_entry.expiration == cherokee_expiration_epoch) {
							req->flcache.avl_node_ref->valid_until = 0;
						} else if (req->config_entry.expiration == cherokee_expiration_time) {
							req->flcache.avl_node_ref->valid_until = cherokee_bogonow_now + req->config_entry.expiration_time;
						}
					}
				}

				req->phase = phase_init;
			}

			case phase_init:
				/* Look for the request
				 */
				ret = cherokee_request_open_request (req);
				switch (ret) {
				case ret_ok:
				case ret_error:
					break;

				case ret_eagain:
					continue;

				default:
					if ((MODULE(req->handler)->info) &&
					    (MODULE(req->handler)->info->name))
						LOG_ERROR (CHEROKEE_ERROR_THREAD_HANDLER_RET,
							   ret, MODULE(req->handler)->info->name);
					else
						RET_UNKNOWN(ret);
					break;
				}

				/* If it is an error, and the connection has not a handler to manage
				 * this error, the handler has to be changed by an error_handler.
				 */
				if (req->handler == NULL) {
					goto shutdown;
				}

				if (http_type_300(req->error_code) ||
				    http_type_400(req->error_code) ||
				    http_type_500(req->error_code))
				{
					if (HANDLER_SUPPORTS (req->handler, hsupport_error)) {
						ret = cherokee_request_clean_error_headers (req);
						if (unlikely (ret != ret_ok)) {
							goto shutdown;
						}
					} else {
						/* Try to setup an error handler
						 */
						ret = cherokee_request_setup_error_handler (req);
						if ((ret != ret_ok) &&
						    (ret != ret_eagain))
						{
							/* Critical error: It couldn't instance the handler
							 */
							goto shutdown;
						}
						continue;
					}
				}

				/* Figure next state
				 */
				if (! (http_method_with_input (req->header.method) ||
				       http_method_with_optional_input (req->header.method)))
				{
					req->phase = phase_add_headers;
					goto add_headers;
				}

				/* Register with the POST tracker
				 */
				if ((srv->post_track) && (req->post.has_info)) {
					srv->post_track->func_register (srv->post_track, req);
				}

				req->phase = phase_reading_post;

			case phase_reading_post:

				/* Read/Send the POST info
				 */
				ret = cherokee_request_read_post (req);
				switch (ret) {
				case ret_ok:
					break;
				case ret_eagain:
					if (cherokee_request_poll_is_set (&req->polling_aim)) {
						cherokee_thread_deactive_to_polling (thd, req);
					}
					continue;
				case ret_eof:
				case ret_error:
					req->error_code = http_internal_error;
					cherokee_request_setup_error_handler (req);
					continue;
				default:
					RET_UNKNOWN(ret);
				}

				/* Turn the connection in write mode
				 */
				req->phase = phase_add_headers;

			case phase_add_headers:
			add_headers:

				/* Build the header
				 */
				ret = cherokee_request_build_header (req);
				switch (ret) {
				case ret_ok:
					break;
				case ret_eagain:
					if (cherokee_request_poll_is_set (&req->polling_aim)) {
						cherokee_thread_deactive_to_polling (thd, req);
					}
					continue;
				case ret_eof:
				case ret_error:
					req->error_code = http_internal_error;
					cherokee_request_setup_error_handler (req);
					continue;
				default:
					RET_UNKNOWN(ret);
				}

				/* If it is an error, we have to respin the connection
				 * to install a proper error handler.
				 */
				if ((http_type_300 (req->error_code) ||
				     http_type_400 (req->error_code) ||
				     http_type_500 (req->error_code)) &&
				    (!HANDLER_SUPPORTS (req->handler, hsupport_error))) {
					req->phase = phase_setup_connection;
					continue;
				}

				/* If it has mmaped content, skip next stage
				 */
				if (req->mmaped != NULL)
					goto phase_send_headers_EXIT;

				/* Front-line cache: store
				 */
				if (req->flcache.mode == flcache_mode_in) {
					ret = cherokee_flcache_conn_commit_header (&req->flcache, req);
					if (ret != ret_ok) {
						/* Disabled Front-Line Cache */
						req->flcache.mode = flcache_mode_error;
					}
				}

				req->phase = phase_send_headers;

			case phase_send_headers:

				/* Send headers to the client
				 */
				ret = cherokee_request_send_header (req);
				switch (ret) {
				case ret_eagain:
					cherokee_thread_deactive_to_polling (thd, req);
					continue;

				case ret_ok:
					if (!http_method_with_body (req->header.method)) {
						finalize_request (thd, req);
						continue;
					}
					if (!http_code_with_body (req->error_code)) {
						finalize_request (thd, req);
						continue;
					}
					break;

				case ret_eof:
				case ret_error:
					goto shutdown;

				default:
					RET_UNKNOWN(ret);
				}

			phase_send_headers_EXIT:
				req->phase = phase_stepping;

			case phase_stepping:

				/* Special case:
				 * If the content is mmap()ed, it has to send the header +
				 * the file content and stop processing the connection.
				 */
				if (req->mmaped != NULL) {
					ret = cherokee_request_send_header_and_mmaped (req);
					switch (ret) {
					case ret_eagain:
						cherokee_thread_deactive_to_polling (thd, req);
						continue;

					case ret_eof:
						finalize_request (thd, req);
						continue;

					case ret_error:
						close_active_connection (thd, req, true);
						continue;

					default:
						finalize_request (thd, req);
						continue;
					}
				}

				/* Handler step: read or make new data to send
				 * Front-line cache handled internally.
				 */
				ret = cherokee_request_step (req);
				switch (ret) {
				case ret_eagain:
					if (cherokee_request_poll_is_set (&req->polling_aim)) {
						cherokee_thread_deactive_to_polling (thd, req);
					}
					continue;

				case ret_eof_have_data:
					ret = cherokee_request_send (req);

					switch (ret) {
					case ret_ok:
						finalize_request (thd, req);
						continue;
					case ret_eagain:
						if (cherokee_request_poll_is_set (&req->polling_aim)) {
							cherokee_thread_deactive_to_polling (thd, req);
						}
						break;
					case ret_eof:
					case ret_error:
					default:
						close_active_connection (thd, req, false);
						continue;
					}
					break;

				case ret_ok:
					ret = cherokee_request_send (req);

					switch (ret) {
					case ret_ok:
						continue;
					case ret_eagain:
						if (cherokee_request_poll_is_set (&req->polling_aim)) {
							cherokee_thread_deactive_to_polling (thd, req);
						}
						break;
					case ret_eof:
					case ret_error:
					default:
						close_active_connection (thd, req, false);
						continue;
					}
					break;

				case ret_ok_and_sent:
					break;

				case ret_eof:
					finalize_request (thd, req);
					continue;

				case ret_error:
					close_active_connection (thd, req, false);
					continue;

				default:
					RET_UNKNOWN(ret);
					goto shutdown;
				}
				break;

			shutdown:
				req->phase = phase_shutdown;

			case phase_shutdown:
				/* Perform a proper SSL/TLS shutdown
				 */
				if (req->socket.is_tls == TLS) {
					ret = req->socket.cryptor->shutdown (req->socket.cryptor);
					switch (ret) {
					case ret_ok:
					case ret_eof:
					case ret_error:
						break;

					case ret_eagain:
						cherokee_thread_deactive_to_polling (thd, req);
						continue;

					default:
						RET_UNKNOWN (ret);
						close_active_connection (thd, req, false);
						continue;
					}
				}

				/* Shutdown socket for writing
				 */
				ret = cherokee_request_shutdown_wr (req);
				switch (ret) {
				case ret_ok:
					/* Extend the timeout
					 */
					conn->timeout = cherokee_bogonow_now + SECONDS_TO_LINGER;
					TRACE (ENTRIES, "Lingering-close timeout = now + %d secs\n", SECONDS_TO_LINGER);

					/* Wait for the socket to be readable:
					 * FIN + ACK will have arrived by then
					 */
					req->phase = phase_lingering;

					break;
				default:
					/* Error, no linger and no last read,
					 * just close the connection.
					 */
					close_active_connection (thd, req, true);
					continue;
				}

				/* fall down */

			case phase_lingering:
				ret = cherokee_request_linger_read (req);
				switch (ret) {
				case ret_ok:
					continue;
				case ret_eagain:
					cherokee_thread_deactive_to_polling (thd, req);
					continue;
				case ret_eof:
				case ret_error:
					close_active_connection (thd, req, false);
					continue;
				default:
					RET_UNKNOWN(ret);
					close_active_connection (thd, req, false);
					continue;
				}
				break;

			default:
				SHOULDNT_HAPPEN;
			}
		} /* list (req) */
	} /* list (conn)*/
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
		cherokee_request_free (REQ(i));
	}

	list_for_each_safe (i, tmp, &thd->reuse_list) {
		cherokee_request_free (REQ(i));
	}

	cherokee_limiter_mrproper (&thd->limiter);

	/* FastCGI
	 */
	if (thd->fastcgi_servers != NULL) {
		cherokee_avl_free (AVL_GENERIC(thd->fastcgi_servers), thd->fastcgi_free_func);
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
	if (cherokee_fdpoll_check (thd->fdpoll, S_SOCKET_FD(bind->socket), poll_mode_read) <= 0) {
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
get_new_connection (cherokee_thread_t      *thd,
		    cherokee_connection_t **conn)
{
	cherokee_connection_t *new_connection;
	cherokee_server_t     *server          = SRV(thd->server);
	static cuint_t         last_conn_id    = 0;

	/* New connection object */
	ret = cherokee_thread_get_new_connection (thd, &new_connection);
	if (unlikely(ret < ret_ok)) {
		return ret;
	}

	/* Set the basic information to the connection
	 */
	new_connection->id           = last_conn_id++;
	new_connection->thread       = thd;
	new_connection->server       = server;
	new_connection->vserver      = VSERVER(server->vservers.prev);
	new_connection->traffic_next = cherokee_bogonow_now + DEFAULT_TRAFFIC_UPDATE;

	/* Set the default server timeout
	 */
	new_connection->timeout        = cherokee_bogonow_now + server->timeout;
	new_connection->timeout_lapse  = server->timeout;
	new_connection->timeout_header = &server->timeout_header;

	*conn = new_connection;
	return ret_ok;
}


static ret_t
accept_new_connection (cherokee_thread_t *thd,
		       cherokee_bind_t   *bind)
{
	int                  re;
	ret_t                ret;
	cherokee_sockaddr_t  new_sa;
	cherokee_request_t  *new_conn  = NULL;
	int                  new_fd    = -1;
	cherokee_server_t   *srv       = THREAD_SRV(thd);
	cherokee_boolean_t   lock_set  = false;

	/* Check whether there are connections waiting
	 */
	re = cherokee_fdpoll_check (thd->fdpoll, S_SOCKET_FD(bind->socket), poll_mode_read);
	if (re <= 0) {
		return ret_deny;
	}

	/* Try to get a new connection
	 */
	ret = cherokee_socket_accept_fd (&bind->socket, &new_fd, &new_sa);
	if ((ret != ret_ok) || (new_fd == -1)) {
		return ret_deny;
	}

	/* Information collection
	 */
	if (srv->collector != NULL) {
		cherokee_collector_log_accept (srv->collector);
	}

	/* We got the new socket, now set it up in a new connection object
	 */
	ret = get_new_connection (thd, &new_conn);
	if (unlikely(ret < ret_ok)) {
		LOG_ERROR_S (CHEROKEE_ERROR_THREAD_GET_CONN_OBJ);
		goto error;
	}

	/* Set the actual fd info in the connection
	 */
	ret = cherokee_socket_set_sockaddr (&new_conn->socket, new_fd, &new_sa);
	if (unlikely(ret < ret_ok)) {
		LOG_ERROR_S (CHEROKEE_ERROR_THREAD_SET_SOCKADDR);
		goto error;
	}

	/* It is about to add a new connection to the thread,
	 * so it MUST adquire the thread ownership
	 * (do it now to better handle error cases).
	 */
	lock_set = true;
	CHEROKEE_MUTEX_LOCK (&thd->ownership);

	/* TLS support, set initial connection phase.
	 */
	if (bind->socket.is_tls == TLS) {
		new_conn->phase = phase_tls_handshake;

		/* Set a custom timeout for the handshake
		 */
		if ((srv->cryptor != NULL) &&
		    (srv->cryptor->timeout_handshake > 0))
		{
			new_conn->timeout       = cherokee_bogonow_now + srv->cryptor->timeout_handshake;
			new_conn->timeout_lapse = srv->cryptor->timeout_handshake;
		}
	}

	/* Set the reference to the port
	 */
	new_conn->bind = bind;

	/* Lets add the new connection
	 */
	add_connection (thd, new_conn);
	thd->conns_num++;

	/* Release the thread ownership
	 */
	CHEROKEE_MUTEX_UNLOCK (&thd->ownership);

	TRACE (ENTRIES, "new conn %p, fd %d\n", new_conn, new_fd);
	return ret_ok;

error:
	TRACE (ENTRIES, "error accepting connection fd %d from port %d\n",
	       new_fd, bind->port);

	/* Close new socket
	 */
	cherokee_fd_close (new_fd);

	/* Recycle the new_conn object
	 */
	if (new_conn) {
		S_SOCKET_FD(new_conn->socket) = -1;
		cherokee_thread_recycle_connection (thd, new_conn);
	}

	/* Release the thread ownership
	 */
	if (lock_set) {
		CHEROKEE_MUTEX_UNLOCK (&thd->ownership);
	}
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

	/* May have to reactive connections
	 */
	cherokee_limiter_reactive (&thd->limiter, thd);

	/* Quick polling if there are active connections
	 */
	if (! cherokee_list_empty (&thd->active_list)) {
		fdwatch_msecs = 0;
	}

	/* Reactive sleeping connections
	 */
	fdwatch_msecs = cherokee_limiter_get_time_limit (&thd->limiter,
							 fdwatch_msecs);

	/* Graceful restart
	 */
	if (unlikely (srv->wanna_reinit)) {
		if (cherokee_list_empty (&thd->active_list) &&
		    cherokee_list_empty (&thd->polling_list))
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
	process_active_connections (thd);

	return ret_ok;
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
		       (cherokee_list_empty (&thd->active_list)) &&
		       (cherokee_list_empty (&thd->polling_list)))))
	{
		goto out;
	}

	/* Locked; Add port file descriptors
	 */
	list_for_each (i, &srv->listeners) {
		ret = cherokee_fdpoll_add (thd->fdpoll,
					   S_SOCKET_FD(BIND(i)->socket),
					   poll_mode_read);
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

	/* Quick polling if there are active connections
	 */
	if (! cherokee_list_empty (&thd->active_list)) {
		fdwatch_msecs = 0;
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
		if ((cherokee_list_empty (&thd->active_list)) &&
		    (cherokee_list_empty (&thd->polling_list)))
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
		     (thd->limiter.conns_num == 0) &&
		     (cherokee_list_empty (&thd->active_list)) &&
		     (cherokee_list_empty (&thd->polling_list)));

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
	process_active_connections (thd);

	/* Release the thread
	 */
	CHEROKEE_MUTEX_UNLOCK (&thd->ownership);

	return ret_ok;
}

#endif /* HAVE_PTHREAD */


int
cherokee_thread_connection_num (cherokee_thread_t *thd)
{
	size_t len = 0;

	cherokee_list_get_len (&thd->active_list, &len);

	return len;
}


ret_t
cherokee_thread_close_polling_connections (cherokee_thread_t *thd, int fd, cuint_t *num)
{
	cuint_t             n = 0;
	cherokee_list_t    *i, *tmp;
	cherokee_request_t *req;

	list_for_each_safe (i, tmp, &thd->polling_list) {
		req = REQ(i);

		if (req->polling_aim.fd == fd) {
			purge_closed_polling_connection (thd, req);
			n++;
		}
	}

	if (num != NULL) {
		*num = n;
	}
	return ret_ok;
}


/* Interface for handlers:
 * It could want to add a file descriptor to the thread fdpoll
 */

static ret_t
move_connection_to_polling (cherokee_thread_t *thd, cherokee_request_t *req)
{
	del_connection (thd, req);
	add_connection_polling (thd, req);

	return ret_ok;
}


static ret_t
move_connection_to_active (cherokee_thread_t *thd, cherokee_request_t *req)
{
	del_connection_polling (thd, req);
	add_connection (thd, req);

	return ret_ok;
}


static ret_t
reactive_conn_from_polling (cherokee_thread_t  *thd,
			    cherokee_request_t *req)
{
	ret_t              ret;
	cherokee_socket_t *socket = &req->socket;

	TRACE (ENTRIES",polling", "conn=%p(fd=%d)\n", req, SOCKET_FD(socket));

	/* Set the connection file descriptor and remove the old one
	 */
	ret = cherokee_fdpoll_del (thd->fdpoll, req->polling_aim.fd);
	if (unlikely (ret != ret_ok)) {
		SHOULDNT_HAPPEN;
	}

	/* Reset the 'polling aim' object
	 */
	cherokee_request_poll_clean (&req->polling_aim);

	/* Put connection in the 'active' list
	 */
	return move_connection_to_active (thd, req);
}


ret_t
cherokee_thread_deactive_to_polling (cherokee_thread_t  *thd,
				     cherokee_request_t *req)
{
	ret_t              ret;
	cherokee_socket_t *socket = &req->socket;

	/* If either the 'aim polling' file descriptor or mode
	 * is not set, the connection is not deactived.
	 */
	if ((req->polling_aim.fd < 0) ||
	    (req->polling_aim.mode == poll_mode_nothing))
	{
		SHOULDNT_HAPPEN;
		CHEROKEE_PRINT_BACKTRACE;
		return ret_error;
	}

	TRACE (ENTRIES",polling", "conn=%p(fd=%d) (fd=%d, mode=%s -> polling)\n",
	       req, SOCKET_FD(socket), req->polling_aim.fd,
	       req->polling_aim.mode == poll_mode_read  ? "read"  :
	       req->polling_aim.mode == poll_mode_write ? "write" : "???");

	/* Add the fd to the fdpoll
	 */
	ret = cherokee_fdpoll_add (thd->fdpoll,
				   req->polling_aim.fd,
				   req->polling_aim.mode);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	return move_connection_to_polling (thd, req);
}


ret_t
cherokee_thread_retire_active_connection (cherokee_thread_t  *thd,
					  cherokee_request_t *req)
{
	del_connection (thd, req);
	return ret_ok;
}


ret_t
cherokee_thread_inject_active_connection (cherokee_thread_t  *thd,
					  cherokee_request_t *req)
{
	add_connection (thd, req);
	return ret_ok;
}
