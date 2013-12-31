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

#ifndef CHEROKEE_THREAD_H
#define CHEROKEE_THREAD_H

#include "common-internal.h"

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include "list.h"
#include "socket.h"
#include "connection.h"
#include "connection-protected.h"
#include "fdpoll.h"
#include "avl.h"
#include "limiter.h"


typedef enum {
	thread_sync,
	thread_async
} cherokee_thread_type_t;


typedef struct {
	cherokee_list_t         base;

#ifdef HAVE_PTHREAD
	pthread_t               thread;
	pthread_mutex_t         starting_lock;
	pthread_mutex_t         ownership;
#endif

	cherokee_fdpoll_t      *fdpoll;
	cherokee_thread_type_t  thread_type;

	time_t                  bogo_now;
	struct tm               bogo_now_tmgmt;
	struct tm               bogo_now_tmloc;
	cherokee_buffer_t       bogo_now_strgmt;

	cherokee_buffer_t       tmp_buf1;
	cherokee_buffer_t       tmp_buf2;

	void                   *server;
	cherokee_boolean_t      exit;
	cherokee_boolean_t      ended;

	cuint_t                 conns_num;           /* open connections */
	cuint_t                 conns_max;           /* max opened conns */
	cuint_t                 conns_keepalive_max; /* max opened conns */

	int                     active_list_num;     /* active connections */
	cherokee_list_t         active_list;
	int                     polling_list_num;    /* polling connections */
	cherokee_list_t         polling_list;
	cherokee_list_t         reuse_list;
	int                     reuse_list_num;      /* reusable connections objs */
	cherokee_limiter_t      limiter;             /* Traffic shaping */
	cherokee_boolean_t      is_full;

	int                     pending_conns_num;   /* Waiting pipelining connections */
	int                     pending_read_num;    /* Conns with SSL deping read */

	cherokee_avl_t         *fastcgi_servers;
	cherokee_func_free_t    fastcgi_free_func;

} cherokee_thread_t;

#define THREAD(x)          ((cherokee_thread_t *)(x))
#define THREAD_SRV(t)      (SRV(THREAD(t)->server))
#define THREAD_IS_REAL(t)  (THREAD(t)->real_thread)
#define THREAD_TMP_BUF1(t) (&THREAD(t)->tmp_buf1)
#define THREAD_TMP_BUF2(t) (&THREAD(t)->tmp_buf2)


#ifdef HAVE_PTHREAD
ret_t cherokee_thread_step_MULTI_THREAD  (cherokee_thread_t *thd, cherokee_boolean_t dont_block);
#else
# define cherokee_thread_step_MULTI_THREAD(t,b) cherokee_thread_step_SINGLE_THREAD(t)
#endif
ret_t cherokee_thread_step_SINGLE_THREAD (cherokee_thread_t *thd);


ret_t cherokee_thread_new                        (cherokee_thread_t     **thd,
                                                  void                   *server,
                                                  cherokee_thread_type_t  type,
                                                  cherokee_poll_type_t    fdtype,
                                                  cint_t                  system_fd_num,
                                                  cint_t                  fds_max,
                                                  cint_t                  conns_max,
                                                  cint_t                  keepalive_max);

ret_t cherokee_thread_free                       (cherokee_thread_t  *thd);

ret_t cherokee_thread_unlock                     (cherokee_thread_t *thd);
ret_t cherokee_thread_wait_end                   (cherokee_thread_t *thd);

ret_t cherokee_thread_deactive_to_polling        (cherokee_thread_t *thd, cherokee_connection_t *conn, int fd, int rw, char multi);
int   cherokee_thread_connection_num             (cherokee_thread_t *thd);

ret_t cherokee_thread_retire_active_connection   (cherokee_thread_t *thd, cherokee_connection_t *conn);
ret_t cherokee_thread_inject_active_connection   (cherokee_thread_t *thd, cherokee_connection_t *conn);

ret_t cherokee_thread_close_all_connections      (cherokee_thread_t *thd);
ret_t cherokee_thread_close_polling_connections  (cherokee_thread_t *thd, int fd, cuint_t *num);

#endif /* CHEROKEE_THREAD_H */
