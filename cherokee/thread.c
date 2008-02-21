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
#include "reqs_list_entry.h"
#include "util.h"
#include "fcgi_manager.h"


#define DEBUG_BUFFER(b)  fprintf(stderr, "%s:%d len=%d crc=%d\n", __FILE__, __LINE__, b->len, cherokee_buffer_crc32(b))
#define ENTRIES "core,thread"

static ret_t reactive_conn_from_polling  (cherokee_thread_t *thd, cherokee_connection_t *conn);


static char *
phase_to_str (cherokee_connection_phase_t phase)
{
	switch (phase) {
	case phase_nothing:           return "Nothing";
	case phase_switching_headers: return "Switch headers";
	case phase_tls_handshake:     return "TLS handshake";
	case phase_reading_header:    return "Reading header";
	case phase_processing_header: return "Processing header";
	case phase_read_post:         return "Read POST";
	case phase_setup_connection:  return "Setup connection";
	case phase_init:              return "Init connection";
	case phase_add_headers:       return "Add headers";
	case phase_send_headers:      return "Send headers";
	case phase_steping:           return "Step";
	case phase_shutdown:          return "Shutdown connection";
	case phase_lingering:         return "Lingering close";
	default:
		SHOULDNT_HAPPEN;
	}
	return NULL;
}

static void
update_bogo_now_internal (cherokee_thread_t *thd)
{
	cherokee_server_t *srv = THREAD_SRV(thd);

	/* Has it changed?
	 */
	if (thd->bogo_now == srv->bogo_now)
		return;

	/* Update time_t
	 */
	thd->bogo_now = srv->bogo_now;

	/* Update struct tm
	 */
	memcpy (&thd->bogo_now_tmgmt, &srv->bogo_now_tmgmt, sizeof(struct tm));
	memcpy (&thd->bogo_now_tmloc, &srv->bogo_now_tmloc, sizeof(struct tm));

	/* Update cherokee_buffer_t
	 */
	cherokee_buffer_clean (&thd->bogo_now_strgmt);
	cherokee_buffer_add_buffer (&thd->bogo_now_strgmt, &srv->bogo_now_strgmt);

}


static void
update_bogo_now (cherokee_thread_t *thd)
{
	CHEROKEE_RWLOCK_READER (&THREAD_SRV(thd)->bogo_now_mutex);
	update_bogo_now_internal(thd);
	CHEROKEE_RWLOCK_UNLOCK (&THREAD_SRV(thd)->bogo_now_mutex);
}


static void
try_to_update_bogo_now (cherokee_thread_t *thd)
{
	int unlocked;
	cherokee_server_t *srv = THREAD_SRV(thd);

	/* Try to lock
	 */
	unlocked = CHEROKEE_RWLOCK_TRYREADER (&srv->bogo_now_mutex);        /* 1.- lock a reader */
	if (unlocked) return;

	update_bogo_now_internal (thd);

	CHEROKEE_RWLOCK_UNLOCK (&srv->bogo_now_mutex);                      /* 2.- release */
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
	update_bogo_now (thread);

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
#ifdef HAVE_PTHREAD
	/* Wait until the thread exits
	 */
	pthread_join (thd->thread, NULL);
#endif
	return ret_ok;	
}


ret_t 
cherokee_thread_new  (
	cherokee_thread_t **thd, void *server, cherokee_thread_type_t type, 
	cherokee_poll_type_t fdpoll_type, int system_fd_num, int fd_num,
	int conns_max)
{
	ret_t              ret;
	int                conns_accept;
	cherokee_server_t *srv = SRV(server);
	CHEROKEE_CNEW_STRUCT (1, n, thread);

	/* Init
	 */
	INIT_LIST_HEAD (LIST(&n->base));
	INIT_LIST_HEAD (LIST(&n->active_list));
	INIT_LIST_HEAD (LIST(&n->reuse_list));
	INIT_LIST_HEAD (LIST(&n->polling_list));
	
	if (fdpoll_type == cherokee_poll_UNSET)
		ret = cherokee_fdpoll_best_new (&n->fdpoll, system_fd_num, fd_num);
	else
		ret = cherokee_fdpoll_new (&n->fdpoll, fdpoll_type, system_fd_num, fd_num);

	if (unlikely (ret != ret_ok)) {
		CHEROKEE_FREE(n);
		return ret;
	}

	n->exit              = false;
	n->is_accepting_conns= false;

	if (fd_num < conns_max ||
	    conns_max < 4) {
		cherokee_fdpoll_free (n->fdpoll);
		CHEROKEE_FREE(n);
		return ret_error;
	}

	/* Set upper accept limit to 95% - 99% of conns_max.
	 * TODO: change the following hardcoded limits into a formula.
	 */
	if (conns_max > 40) {
		conns_accept = conns_max - (conns_max / 20);

		if (srv->thread_num > 16) {
			if (conns_accept < (conns_max - 4))
				conns_accept = (conns_max - 4);
		} else
		if (srv->thread_num > 4) {
			if (conns_accept < (conns_max - 8))
				conns_accept = (conns_max - 8);
		} else
		if (srv->thread_num > 1) {
			if (conns_accept < (conns_max - 10))
				conns_accept = (conns_max - 10);
		} else {
			if (conns_accept < (conns_max - 50))
				conns_accept = (conns_max - 50);
		}
		if (conns_accept == conns_max)
			conns_accept -= 2;
	} else {
		conns_accept    = conns_max - 2;
	}

	n->conns_num         = 0;
	n->conns_max         = conns_max;
	n->conns_accept      = conns_accept;

	n->active_list_num   = 0;
	n->polling_list_num  = 0;
	n->reuse_list_num    = 0;

	n->pending_conns_num = 0;

	n->server            = server;
	n->thread_type       = type;

	n->fastcgi_servers   = NULL;
	n->fastcgi_free_func = NULL;

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
	 *  problem, otherwise it may avoid a nasty connection reset.
	 */
	if (conn->phase == phase_lingering) {
		cherokee_connection_linger_read (conn);
	}

	/* Maybe have a delayed log
	 */
	cherokee_connection_log_delayed (conn);

	/* close & clean the socket and clean up the connection object
	 */
	cherokee_connection_mrproper (conn);

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

	/* TCP cork
	 */
	if (conn->options & conn_op_tcp_cork) {
		cherokee_connection_set_cork (conn, false);
	}

	/* Clean the connection
	 */
	cherokee_connection_clean (conn);
	conn_set_mode (thread, conn, socket_reading);

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
		re = cherokee_fdpoll_check (thd->fdpoll, conn->polling_fd, FDPOLL_MODE_READ);
		switch (re) {
		case -1:
			/* Error, move back the connection
			 */
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

		TRACE (ENTRIES, "thread (%p) processing conn (%p), phase %d\n", thd, conn, conn->phase);

		/* Has the connection been too much time w/o any work
		 */
		if (conn->timeout < thd->bogo_now) {
			conns_freed++;
			purge_closed_connection (thd, conn);
			continue;
		}

		/* Maybe update traffic counters
		 */
		if ((conn->traffic_next < thd->bogo_now) &&
		    (conn->rx != 0) &&
		    (conn->tx != 0)) {
			cherokee_connection_update_vhost_traffic (conn);
		}

		/* Process the connection?
		 * 2.- Inspect the file descriptor if it's not shutdown
		 *     and it's not reading header or there is no more buffered data.
		 */
		if ( conn->phase != phase_shutdown &&
			(conn->phase != phase_reading_header ||
			 conn->incoming_header.len < 1)) {
			int num;

			num = cherokee_fdpoll_check (thd->fdpoll, 
						     SOCKET_FD(&conn->socket), 
						     SOCKET_STATUS(&conn->socket));
			switch (num) {
			case -1:
				conns_freed++;
				purge_closed_connection(thd, conn);
				continue;
			case 0:
				continue;
			}
		}

		/* The connection has work to do, so do it ...
		 */
		conn->timeout = thd->bogo_now + srv->timeout;

		TRACE (ENTRIES, "conn on phase n=%d: %s\n", conn->phase, phase_to_str(conn->phase));

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
			if (! cherokee_buffer_is_empty (&conn->incoming_header)) {
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
			cherokee_config_entry_t     entry; 
			cherokee_virtual_entries_t *ventry;
			cherokee_boolean_t          matched_req;
			cherokee_boolean_t          is_userdir;

			TRACE (ENTRIES, "Setup connection begins: request=\"%s\"\n", conn->request.buf);

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
				ventry = &CONN_VSRV(conn)->userdir_entry;
			} else {
				ventry = &CONN_VSRV(conn)->entry;
			}
			
			/* 1.- Read the extension configuration
			 */
			ret = cherokee_connection_get_ext_entry (conn, &ventry->exts, &entry);
			if (unlikely (ret != ret_ok)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}

			/* 2.- Read the directory configuration
			 */
			ret = cherokee_connection_get_dir_entry (conn, &ventry->dirs, &entry);
			switch (ret) {
			case ret_not_found:
				break;
			case ret_ok:
				if (cherokee_buffer_is_empty (&conn->local_directory)) {
					if (is_userdir)
						ret = cherokee_connection_build_local_directory_userdir (conn, CONN_VSRV(conn), &entry);
					else
						ret = cherokee_connection_build_local_directory (conn, CONN_VSRV(conn), &entry);
				}
				break;
			default:
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}

			if (unlikely (ret == ret_error)) {
				cherokee_connection_setup_error_handler (conn);
				conn->phase = phase_init;
				continue;
			}

			/* 3.- Read Request configurations
			 */
			matched_req = false;
			if (! cherokee_list_empty (&ventry->reqs)) {
				ret = cherokee_connection_get_req_entry (conn, &ventry->reqs, &entry);
				switch (ret) {
				case ret_ok:
					matched_req = true;
					break;
				case ret_not_found:
					break;
				default:
					cherokee_connection_setup_error_handler (conn);
					conn->phase = phase_init;
					continue;
				}				
			}			

			/* 4.- Default handler check
			 */
			if (CONN_VSRV(conn)->default_handler != NULL) {
				cherokee_virtual_server_t *vsrv = CONN_VSRV(conn);
 
				cherokee_config_entry_complete (&entry, vsrv->default_handler, false);

				if (conn->realm_ref == NULL) 
					conn->realm_ref = vsrv->default_handler->auth_realm;
				if (conn->auth_type == http_auth_nothing) 
					conn->auth_type = vsrv->default_handler->authentication;

				if (cherokee_buffer_is_empty (&conn->web_directory) && (! matched_req)) {
					cherokee_buffer_add_str (&conn->web_directory, "/");
				}
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
			
			/* Parse the rest of headers
			 */
			ret = cherokee_connection_parse_header (conn, &srv->encoders);
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
			/* Server's "Keep-Alive" could be turned "Off"
			 */
			if (conn->keepalive != 0) {
				if ((srv->keepalive == false) ||
				    ((srv->conns_num_bogo - conns_freed) >= srv->conns_keepalive_max))
					conn->keepalive = 0;
			}

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
			 * this error, the handler has to be changed
			 */
 			if ((http_type_300(conn->error_code) || 
			     http_type_400(conn->error_code) ||
			     http_type_500(conn->error_code)) &&
			    conn->handler && (!HANDLER_SUPPORTS (conn->handler, hsupport_error)))
			{
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
				if (http_type_300(conn->error_code)) {
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
					/* purge_maybe_lingering (thd, conn); */
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
					/* purge_maybe_lingering (thd, conn); */
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


static int
__accept_from_server (cherokee_thread_t *thd, int srv_socket, cherokee_socket_type_t tls)
{
	ret_t                  ret;
	int                    new_fd;
	cherokee_sockaddr_t    new_sa;
	cherokee_connection_t *new_conn;

	/* Return if there're too many or no new connections
	 */
	if (thd->conns_num >= thd->conns_max)
		return 0;

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
		cherokee_close_fd (new_fd);
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
	cherokee_close_fd (new_fd);
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
__should_accept_more_from_server (cherokee_thread_t *thd, int re)
{
	const uint32_t recalculate_steps = 10;

	/* If it is full, do not accept more!
	 */
	if (unlikely (thd->conns_num >= thd->conns_max))
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
cherokee_thread_accept_on (cherokee_thread_t *thd)
{
	ret_t ret;

	if (thd->is_accepting_conns)
		return ret_ok;

	/* Add server sockets to polling set.
	 */
	ret = cherokee_fdpoll_add (thd->fdpoll,
		S_SOCKET_FD(THREAD_SRV(thd)->socket), FDPOLL_MODE_READ);
	if (unlikely(ret < ret_ok))
		return ret;

#ifdef HAVE_TLS
	if (THREAD_SRV(thd)->tls_enabled) {
		ret = cherokee_fdpoll_add (thd->fdpoll,
			S_SOCKET_FD(THREAD_SRV(thd)->socket_tls), FDPOLL_MODE_READ);
		if (unlikely(ret < ret_ok))
			return ret;
	}
#endif /* HAVE_TLS */

	/* Set flag and return.
	 */
	thd->is_accepting_conns = true;

	return ret_ok;
}


ret_t 
cherokee_thread_accept_off (cherokee_thread_t *thd)
{
	ret_t ret;

	if (!thd->is_accepting_conns)
		return ret_ok;

	/* Remove server sockets from polling set.
	 */
	ret = cherokee_fdpoll_del (thd->fdpoll,
		S_SOCKET_FD(THREAD_SRV(thd)->socket));
	if (unlikely(ret < ret_ok))
		return ret;

#ifdef HAVE_TLS
	if (THREAD_SRV(thd)->tls_enabled) {
		ret = cherokee_fdpoll_del (thd->fdpoll,
			S_SOCKET_FD(THREAD_SRV(thd)->socket_tls));
		if (unlikely(ret < ret_ok))
			return ret;
	}
#endif /* HAVE_TLS */

	/* Set flag and return.
	 */
	thd->is_accepting_conns = false;

	return ret_ok;
}


ret_t 
cherokee_thread_step_SINGLE_THREAD (cherokee_thread_t *thd)
{
	int                re;
	ret_t              ret;
	cherokee_server_t *srv           = THREAD_SRV(thd);
	int                fdwatch_msecs = srv->fdwatch_msecs;

	/* Try to update bogo_now
	 */
	try_to_update_bogo_now (thd);

	/* Reset the server socket.
	 * cherokee_fdpoll_reset (thd->fdpoll, S_SOCKET_FD(srv->socket));
	 */

	/* If the thread is full of connections, it should not
	 * get new connections.
	 */
	if (thd->conns_num >= thd->conns_max) {
		if (thd->is_accepting_conns)
			ret = cherokee_thread_accept_off (thd);
	}
	else
	if (thd->conns_num < thd->conns_accept) {
		if (!thd->is_accepting_conns)
			ret = cherokee_thread_accept_on (thd);
	}

#if 0
	if (unlikely (cherokee_fdpoll_is_full (thd->fdpoll))) {
		goto out;
	}
#endif

	/* If thread has pending connections, it should do a 
	 * faster 'watch' (whenever possible).
	 */
	if (thd->pending_conns_num > 0) {
		fdwatch_msecs          = 0;
		thd->pending_conns_num = 0;
	}

	re = cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	if (re <= 0)
		goto out;

	do {
		re = __accept_from_server (thd, S_SOCKET_FD(srv->socket), non_TLS);
	} while (__should_accept_more_from_server (thd, re));

#ifdef HAVE_TLS
	if (srv->tls_enabled) {
		do {
			re = __accept_from_server (thd, S_SOCKET_FD(srv->socket_tls), TLS);
		} while (__should_accept_more_from_server (thd, re));
	}
#endif /* HAVE_TLS */

out:
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
	int   re;
	ret_t ret;

	/* In this case, thd->is_accepting_conns is alwasy true.
	 */
	CHEROKEE_MUTEX_LOCK (mutex);
	
	ret = cherokee_fdpoll_add (thd->fdpoll, socket, FDPOLL_MODE_READ);
	if (unlikely (ret < ret_ok)) {
		CHEROKEE_MUTEX_UNLOCK (mutex);
		return ret_error;
	}
	/* cherokee_fdpoll_reset (thd->fdpoll, socket);
	 */
	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
	
	/* This thread might be blocked for long long time, so it's
	 * really important to update the local bogo now values before
	 * accepting new connections.  Otherwhise, it will use an old
	 * value for the new connections timeout which might be dropped
	 * in the next step.
	 */
	update_bogo_now (thd);

	do {
		re = __accept_from_server (thd, socket, non_TLS);
	} while (__should_accept_more_from_server (thd, re));

	ret = cherokee_fdpoll_del (thd->fdpoll, socket);
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	CHEROKEE_MUTEX_UNLOCK (mutex);
	
	return ret_ok;
}


static ret_t
step_MULTI_THREAD_nonblock (cherokee_thread_t *thd, int socket, pthread_mutex_t *mutex, int fdwatch_msecs)
{
	int   re;
	ret_t ret;
	int   unlocked = 1;
	
	if (thd->is_accepting_conns) {
		/* Try to lock
		 */
		unlocked = CHEROKEE_MUTEX_TRY_LOCK (mutex);
	}

	if (unlocked) {
		cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
		return ret_ok;
	}

	/* Now it owns the socket.
	 */
	ret = cherokee_fdpoll_add (thd->fdpoll, socket, FDPOLL_MODE_READ);
	if (unlikely (ret < ret_ok)) {
		CHEROKEE_MUTEX_UNLOCK (mutex);
		return ret_error;
	}
	/* cherokee_fdpoll_reset (thd->fdpoll, socket);
	 */
	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);

	do {
		re = __accept_from_server (thd, socket, non_TLS);
	} while (__should_accept_more_from_server (thd, re));

	ret = cherokee_fdpoll_del (thd->fdpoll, socket);
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	CHEROKEE_MUTEX_UNLOCK (mutex);

	return ret_ok;
}

# ifdef HAVE_TLS

static ret_t
step_MULTI_THREAD_TLS_nonblock (cherokee_thread_t *thd, int fdwatch_msecs, 
				int socket,     pthread_mutex_t *mutex, 
				int socket_tls, pthread_mutex_t *mutex_tls)
{
	int   re;
	ret_t ret;
	int   unlock     = 1;
	int   unlock_tls = 1;

	if (thd->is_accepting_conns) {
		/* Try to lock the mutex
		 */
		unlock = CHEROKEE_MUTEX_TRY_LOCK (mutex);
	}
	if (!unlock) {
		ret = cherokee_fdpoll_add (thd->fdpoll, socket, FDPOLL_MODE_READ);
		if (ret < ret_ok) {
			goto error;
		}
		/* cherokee_fdpoll_reset (thd->fdpoll, socket);
		 */
	}

	if (thd->is_accepting_conns) {
		/* Try to lock the TLS mutex
		 */
		unlock_tls = CHEROKEE_MUTEX_TRY_LOCK (mutex_tls);
	}
	if (!unlock_tls) {
		ret = cherokee_fdpoll_add (thd->fdpoll, socket_tls, FDPOLL_MODE_READ);
		if (unlikely (ret < ret_ok)) 
			goto error;

		/* cherokee_fdpoll_reset (thd->fdpoll, socket_tls);
		 */
	}

	/* Inspect the fds and maybe sleep
	 */
	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);
		
	/* Restore..
	 */
	if (!unlock) {
		do {
			re = __accept_from_server (thd, socket, non_TLS);
		} while (__should_accept_more_from_server (thd, re));
		
		ret = cherokee_fdpoll_del (thd->fdpoll, socket);
		if (ret != ret_ok)
			SHOULDNT_HAPPEN;
		CHEROKEE_MUTEX_UNLOCK (mutex);
	}
	
	if (!unlock_tls) {
		do {
			re = __accept_from_server (thd, socket_tls, TLS);
		} while (__should_accept_more_from_server (thd, re));
		
		ret = cherokee_fdpoll_del (thd->fdpoll, socket_tls);
		if (ret != ret_ok)
			SHOULDNT_HAPPEN;
		CHEROKEE_MUTEX_UNLOCK (mutex_tls);
	}
	
	return ret_ok;


error:
	if (!unlock)     CHEROKEE_MUTEX_UNLOCK (mutex);
	if (!unlock_tls) CHEROKEE_MUTEX_UNLOCK (mutex_tls);

	return ret_error;
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

	/* In this case, thd->is_accepting_conns is alwasy true.
	 */

	/* Lock the main mutex
	 */
	CHEROKEE_MUTEX_LOCK (mutex1);

	ret = cherokee_fdpoll_add (thd->fdpoll, socket1, FDPOLL_MODE_READ);
	if (ret < ret_ok) {
		CHEROKEE_MUTEX_UNLOCK (mutex1);
		return ret_error;
	}
	/* cherokee_fdpoll_reset (thd->fdpoll, socket1);
	 */

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
		/* cherokee_fdpoll_reset (thd->fdpoll, socket2);
		 */
	}
#endif

	/* Inspect the fds and get new connections
	 */
	cherokee_fdpoll_watch (thd->fdpoll, fdwatch_msecs);

	/* Update the thread time values before accept new connections.
	 * This ensure a good timeout value for it.
	 */
	update_bogo_now (thd);
		
	do {
		re = __accept_from_server (thd, socket1, type1);
	} while (__should_accept_more_from_server (thd, re));

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
			re = __accept_from_server (thd, socket2, type2);
		} while (__should_accept_more_from_server (thd, re));
		
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
	try_to_update_bogo_now (thd);


	/* If the thread is full of connections, it should not
	 * get new connections.
	 */
	if (thd->conns_num >= thd->conns_max) {
		if (thd->is_accepting_conns)
			thd->is_accepting_conns = false;
	}
	else
	if (thd->conns_num < thd->conns_accept) {
		if (!thd->is_accepting_conns)
			thd->is_accepting_conns = true;
	}

#if 0
	if (unlikely (cherokee_fdpoll_is_full (thd->fdpoll))) {
		goto out;
	}
#endif

	/* If thread has pending connections, it should do a 
	 * faster 'watch' (whenever possible)
	 */
	if (thd->pending_conns_num > 0) {
		fdwatch_msecs          = 0;
		thd->pending_conns_num = 0;
	}

#ifdef HAVE_TLS
	/* Try to get new connections from https
	 */
	if (srv->tls_enabled) {
		if ((thd->exit == false) &&
		    (thd->active_list_num == 0) &&
		    (thd->polling_list_num == 0) && (!dont_block)) {
			step_MULTI_THREAD_TLS_block (thd, fdwatch_msecs, 
			    S_SOCKET_FD(srv->socket), &THREAD_SRV(thd)->accept_mutex, 	
			    S_SOCKET_FD(srv->socket_tls), &THREAD_SRV(thd)->accept_tls_mutex);
		} else {
			step_MULTI_THREAD_TLS_nonblock (thd, fdwatch_msecs, 
			    S_SOCKET_FD(srv->socket), &THREAD_SRV(thd)->accept_mutex, 	
			    S_SOCKET_FD(srv->socket_tls), &THREAD_SRV(thd)->accept_tls_mutex);
		}
		
		goto out;
	}
#endif	

	/* Try to get new connections from http
	 */
	if ((thd->exit == false) &&
	    (thd->active_list_num == 0) && 
	    (thd->polling_list_num == 0) && (!dont_block)) {
		step_MULTI_THREAD_block (thd, S_SOCKET_FD(srv->socket), &THREAD_SRV(thd)->accept_mutex, fdwatch_msecs);
	} else {
		step_MULTI_THREAD_nonblock (thd, S_SOCKET_FD(srv->socket), &THREAD_SRV(thd)->accept_mutex, fdwatch_msecs);
	}
	
out:
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
	new_connection->vserver   = server->vserver_default;
	new_connection->keepalive = server->keepalive_max;

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
cherokee_thread_close_all_connections (cherokee_thread_t *thd)
{
	cherokee_list_t *i, *tmp;
	list_for_each_safe (i, tmp, &thd->active_list) {
		purge_closed_connection (thd, CONN(i));
	}

	return ret_ok;
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

	/* Set the connection file descriptor and remove the old one
	 */
	if (conn->polling_multiple) 
		del = check_removal_multiple_fd (thd, conn->polling_fd);

	if (del) {
		ret = cherokee_fdpoll_del (thd->fdpoll, conn->polling_fd);
		if (ret != ret_ok)
			SHOULDNT_HAPPEN;
	}

/*	printf ("- reactive_conn_from_polling %p, multiple=%d del=%d\n", conn, conn->polling_multiple, del); */

	cherokee_fdpoll_add (thd->fdpoll, socket->socket, socket->status);

	/* Remove the polling fd from the connection
	 */
	conn->polling_fd       = -1;
	conn->polling_multiple = false;

	return move_connection_to_active (thd, conn);
}


ret_t 
cherokee_thread_deactive_to_polling (cherokee_thread_t *thd, cherokee_connection_t *conn, int fd, int rw, char multiple)
{	
	ret_t                  ret;
	cherokee_boolean_t     add_fd = true;
	cherokee_socket_t     *socket = &conn->socket;

	/* Check for fds added more than once
	 */
	if (multiple) 
		add_fd = check_addition_multiple_fd (thd, fd);

/*	printf ("+ move_connection_to_polling %p, multiple=%d add=%d fd=%d\n", conn, multiple, add_fd, fd); */
	
	/* Remove the connection file descriptor and add the new one
	 */
	ret = cherokee_fdpoll_del (thd->fdpoll, SOCKET_FD(socket));
	if (ret != ret_ok)
		SHOULDNT_HAPPEN;

	if (add_fd)
		cherokee_fdpoll_add (thd->fdpoll, fd, rw);

	/* Set the information in the connection
	 */
	conn->polling_fd       = fd;
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
	cherokee_fdpoll_add (thd->fdpoll, SOCKET_FD(&conn->socket), FDPOLL_MODE_WRITE);
	add_connection (thd, conn);

	return ret_ok;
}
