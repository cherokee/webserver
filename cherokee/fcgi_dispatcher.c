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
#include "fcgi_dispatcher.h"
#include "util.h"

#define ENTRIES "dispatcher,cgi"


ret_t 
cherokee_fcgi_dispatcher_new (cherokee_fcgi_dispatcher_t **fcgi, 
			      cherokee_thread_t           *thd, 
			      cherokee_source_t           *src, 
			      cuint_t                      mgr_num, 
			      cuint_t                      keepalive,
			      cuint_t                      pipeline)
{
	ret_t   ret;
	cuint_t i;
        CHEROKEE_NEW_STRUCT (n,fcgi_dispatcher);
	
	/* Init fcgi manager slots
	 */
	INIT_LIST_HEAD(&n->queue);
	CHEROKEE_MUTEX_INIT(&n->lock, NULL);

	n->manager_num = mgr_num;
	n->thread      = thd;

	n->manager = (cherokee_fcgi_manager_t *) malloc (sizeof(cherokee_fcgi_manager_t) * mgr_num);
	if (unlikely(n->manager == NULL)) return ret_nomem;

	TRACE (ENTRIES, "New dispatcher: mgrs=%d, keepalive=%d\n", mgr_num, keepalive);

	for (i=0; i < mgr_num; i++) {
		ret = cherokee_fcgi_manager_init (&n->manager[i], n, src, (i < keepalive), pipeline);
		if (unlikely (ret != ret_ok)) return ret;
	}

	*fcgi = n;
	return ret_ok;
}


ret_t 
cherokee_fcgi_dispatcher_free (cherokee_fcgi_dispatcher_t *fcgi)
{
	cuint_t          i;
	cherokee_list_t *l, *tmp;

	CHEROKEE_MUTEX_DESTROY(&fcgi->lock);
	
	for (i=0; i < fcgi->manager_num; i++) {
		cherokee_fcgi_manager_mrproper (&fcgi->manager[i]);
	}

	list_for_each_safe (l, tmp, &fcgi->queue) {
		cherokee_connection_t *conn = HANDLER_CONN(l);

		cherokee_list_del (LIST(conn));
		cherokee_thread_inject_active_connection (HANDLER_THREAD(l), conn);
	}

	TRACE (ENTRIES, "free: %p\n", fcgi);

	free (fcgi);
	return ret_ok;
}

static ret_t
dispatch (cherokee_fcgi_dispatcher_t  *fcgi, 
	  cherokee_fcgi_manager_t    **mgr_ret)
{
	cuint_t                  i;
	cherokee_fcgi_manager_t *mgr;
	cherokee_boolean_t       found         = false;
 	cuint_t                  lowest_usage = 0; 

#if 0
	printf ("%p [", fcgi);
	for (i=0; i < fcgi->manager_num; i++) {
		printf ("%d ", fcgi->manager[i].conn.len);
	}
	printf ("] queue=%d\n", list_len(&fcgi->queue));
#endif

	/* Look for a suitable manager
	 */
	for (i=0; i < fcgi->manager_num; i++) {
		mgr = &fcgi->manager[i];
		if (mgr->conn.len == 0) {
			TRACE (ENTRIES, "idle manager: %d\n", i);
			
			found = true;
			break;
		}
	}
	
	/* There wasn't an empty slot
	 */
	if (!found) {
		if (! cherokee_fcgi_manager_supports_pipelining (&fcgi->manager[0]))
			return ret_not_found;

		/* Look for the less stressed one
		 */
		mgr = &fcgi->manager[0];
		lowest_usage = mgr->conn.len;

		for (i=1; i < fcgi->manager_num; i++) {
			cherokee_fcgi_manager_t *mgr2 = &fcgi->manager[i];

			if (mgr2->conn.len >= mgr2->pipeline)
				continue;

			if (mgr2->conn.len < lowest_usage) {
				lowest_usage = mgr2->conn.len;
				mgr = mgr2;
			}
		}

		/* There wasn't room for this connection
		 */
		if (lowest_usage >= mgr->pipeline) {
			return ret_not_found;
		}

		TRACE (ENTRIES, "found manager: len %d\n", lowest_usage);

		return ret_not_found;
	} 

	*mgr_ret = mgr;
	return ret_ok;
}


ret_t 
cherokee_fcgi_dispatcher_dispatch (cherokee_fcgi_dispatcher_t  *fcgi, 
				   cherokee_fcgi_manager_t    **mgr_ret)
{
	ret_t ret;

	CHEROKEE_MUTEX_LOCK (&fcgi->lock);
	ret = dispatch (fcgi, mgr_ret);
	CHEROKEE_MUTEX_UNLOCK (&fcgi->lock);

	return ret;
}


ret_t
cherokee_fcgi_dispatcher_end_notif (cherokee_fcgi_dispatcher_t *fcgi)
{
	cherokee_list_t *i;

	if (cherokee_list_empty (&fcgi->queue))
		return ret_ok;

	i = fcgi->queue.next;
	cherokee_list_del (i);

	return cherokee_thread_inject_active_connection (CONN_THREAD(i), CONN(i));
}


ret_t
cherokee_fcgi_dispatcher_queue_conn (cherokee_fcgi_dispatcher_t *fcgi, cherokee_connection_t *conn)
{
	cherokee_list_add_tail (LIST(conn), &fcgi->queue);
	return ret_ok;
}
