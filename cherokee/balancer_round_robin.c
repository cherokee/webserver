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

#include "balancer_round_robin.h"
#include "plugin_loader.h"


/* Plug-in initialization
 */
PLUGIN_INFO_BALANCER_EASIEST_INIT (round_robin);


static ret_t
dispatch (cherokee_balancer_round_robin_t *balancer, 
	  cherokee_connection_t           *conn, 
	  cherokee_source_t              **src);


ret_t 
cherokee_balancer_round_robin_configure (cherokee_balancer_t    *balancer, 
					 cherokee_server_t      *srv, 
					 cherokee_config_node_t *conf)
{
	ret_t ret;

	ret = cherokee_balancer_configure_base (BAL(balancer), srv, conf);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


ret_t 
cherokee_balancer_round_robin_new (cherokee_balancer_t **bal)
{
	CHEROKEE_NEW_STRUCT (n, balancer_round_robin);

	/* Init 	
	 */
	cherokee_balancer_init_base (BAL(n), PLUGIN_INFO_PTR(round_robin));

	MODULE(n)->free   = (module_func_free_t) cherokee_balancer_round_robin_free;
	BAL(n)->configure = (balancer_configure_func_t) cherokee_balancer_round_robin_configure;
	BAL(n)->dispatch  = (balancer_dispatch_func_t) dispatch;

	/* Init properties
	 */
	n->last_one = 0;
	CHEROKEE_MUTEX_INIT (&n->last_one_mutex, NULL);

	/* Return obj
	 */
	*bal = BAL(n);
	return ret_ok;
}


ret_t      
cherokee_balancer_round_robin_free (cherokee_balancer_round_robin_t *balancer)
{
	CHEROKEE_MUTEX_DESTROY (&balancer->last_one_mutex);
	return ret_ok;
}


static ret_t
dispatch (cherokee_balancer_round_robin_t *balancer, 
	  cherokee_connection_t           *conn, 
	  cherokee_source_t              **src)
{
	cherokee_balancer_t *gbal = BAL(balancer);

	UNUSED(conn);
	CHEROKEE_MUTEX_LOCK (&balancer->last_one_mutex);

	if (gbal->sources_len <= 0)
		goto error;
	
	balancer->last_one = (balancer->last_one + 1) % gbal->sources_len;
	*src = gbal->sources[balancer->last_one];

	CHEROKEE_MUTEX_UNLOCK (&balancer->last_one_mutex);
	return ret_ok;

error:
	CHEROKEE_MUTEX_UNLOCK (&balancer->last_one_mutex);
	return ret_error;
}


