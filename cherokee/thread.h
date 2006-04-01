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
#include "table.h"


typedef enum {
	thread_sync, 
	thread_async
} cherokee_thread_type_t;

typedef enum {
	thread_tls_normal,
	thread_normal_tls
} cherokee_thread_pref_t;


typedef struct {
	struct list_head base;

#ifdef HAVE_PTHREAD
	pthread_t               thread;
	pthread_mutex_t         starting_lock;
	pthread_mutex_t         ownership;
#endif

	cherokee_fdpoll_t      *fdpoll;
	cherokee_thread_type_t  thread_type;
	cherokee_thread_pref_t  thread_pref;

	time_t                  bogo_now;
	struct tm               bogo_now_tm;
	cherokee_buffer_t      *bogo_now_string;
	
	void                   *server;
	cherokee_boolean_t      exit;

	int                     active_list_num;
	list_t                  active_list;
	int                     polling_list_num;
	list_t                  polling_list;
	list_t                  reuse_list;
	int                     reuse_list_num;

	int                     pending_conns_num;   /* Waiting pipelining connections */

	struct {
		uint32_t        continuous;
		uint32_t        continuous_max;
		uint32_t        recalculate;		
	} accept;

	cherokee_table_t           *fastcgi_servers;
	cherokee_table_free_item_t  fastcgi_free_func;

} cherokee_thread_t;

#define THREAD(x)         ((cherokee_thread_t *)(x))
#define THREAD_SRV(t)     (SRV(THREAD(t)->server))
#define THREAD_IS_REAL(t) (THREAD(t)->real_thread)


#ifdef HAVE_PTHREAD
# define cherokee_thread_step(t,b) cherokee_thread_step_MULTI_THREAD(t,b)
ret_t cherokee_thread_step_MULTI_THREAD  (cherokee_thread_t *thd, cherokee_boolean_t dont_block);
#else
# define cherokee_thread_step(t,b) cherokee_thread_step_SINGLE_THREAD(t,b)
ret_t cherokee_thread_step_SINGLE_THREAD (cherokee_thread_t *thd, cherokee_boolean_t dont_block);
#endif


ret_t cherokee_thread_new  (cherokee_thread_t **thd, void *server, cherokee_thread_type_t type, cherokee_poll_type_t fdtype, int system_fd_num, int fd_num);
ret_t cherokee_thread_free (cherokee_thread_t  *thd);

ret_t cherokee_thread_unlock                     (cherokee_thread_t *thd);
ret_t cherokee_thread_wait_end                   (cherokee_thread_t *thd);

ret_t cherokee_thread_add_connection             (cherokee_thread_t *thd, cherokee_connection_t  *conn);
ret_t cherokee_thread_get_new_connection         (cherokee_thread_t *thd, cherokee_connection_t **conn);
ret_t cherokee_thread_set_reusable_connection    (cherokee_thread_t *thd, cherokee_connection_t  *conn);

ret_t cherokee_thread_deactive_to_polling        (cherokee_thread_t *thd, cherokee_connection_t *conn, int fd, int rw, char multi);
int   cherokee_thread_connection_num             (cherokee_thread_t *thd);

ret_t cherokee_thread_retire_active_connection   (cherokee_thread_t *thd, cherokee_connection_t *conn);
ret_t cherokee_thread_inject_active_connection   (cherokee_thread_t *thd, cherokee_connection_t *conn);

ret_t cherokee_thread_close_all_connections      (cherokee_thread_t *thd);
ret_t cherokee_thread_close_polling_connections  (cherokee_thread_t *thd, int fd, cuint_t *num);

#endif /* CHEROKEE_THREAD_H */
