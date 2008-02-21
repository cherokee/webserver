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

#ifndef CHEROKEE_FCGI_DISPATCHER_H
#define CHEROKEE_FCGI_DISPATCHER_H

#include "common.h"
#include "fcgi_manager.h"
#include "connection.h"
#include "list.h"
#include "thread.h"


typedef struct {
	cherokee_fcgi_manager_t *manager;
	cuint_t                  manager_num;

	cherokee_list_t          queue;
	cherokee_thread_t       *thread;
	CHEROKEE_MUTEX_T        (lock);
} cherokee_fcgi_dispatcher_t;

#define FCGI_DISPATCHER(f) ((cherokee_fcgi_dispatcher_t *)(f))


ret_t cherokee_fcgi_dispatcher_new        (cherokee_fcgi_dispatcher_t **fcgi, cherokee_thread_t *thd, cherokee_source_t *src, cuint_t mgr_num, cuint_t nkeepalive, cuint_t pipeline);
ret_t cherokee_fcgi_dispatcher_free       (cherokee_fcgi_dispatcher_t  *fcgi);

ret_t cherokee_fcgi_dispatcher_dispatch   (cherokee_fcgi_dispatcher_t *fcgi, cherokee_fcgi_manager_t **mgr);

ret_t cherokee_fcgi_dispatcher_queue_conn (cherokee_fcgi_dispatcher_t *fcgi, cherokee_connection_t *conn); 
ret_t cherokee_fcgi_dispatcher_end_notif  (cherokee_fcgi_dispatcher_t *fcgi);

#endif /* CHEROKEE_FCGI_DISPATCHER_H */
