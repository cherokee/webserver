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

#include "balancer_round_robin.h"
#include "module_loader.h"


ret_t 
cherokee_balancer_round_robin_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, void **props)
{
	return ret_ok;
}

ret_t 
cherokee_balancer_round_robin_new (cherokee_balancer_t **bal, cherokee_connection_t *cnt, cherokee_balancer_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, balancer_round_robin);

	/* Init 	
	 */
	cherokee_balancer_init_base (BAL(n));

	MODULE(n)->free  = (module_func_free_t) cherokee_balancer_round_robin_free;
	BAL(n)->dispatch = (balancer_dispatch_func_t) cherokee_balancer_round_robin_dispatch;

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


ret_t
cherokee_balancer_round_robin_dispatch (cherokee_balancer_round_robin_t *balancer, 
					cherokee_connection_t           *conn, 
					cherokee_balancer_host_t       **host)
{
	cherokee_balancer_t *gbal = BAL(balancer);

	CHEROKEE_MUTEX_LOCK (&balancer->last_one_mutex);

	if (gbal->hosts_len <= 0)
		goto error;
	
	balancer->last_one = (balancer->last_one + 1) % gbal->hosts_len;
	*host = gbal->hosts[balancer->last_one];

	CHEROKEE_MUTEX_UNLOCK (&balancer->last_one_mutex);
	return ret_ok;

error:
	CHEROKEE_MUTEX_UNLOCK (&balancer->last_one_mutex);
	return ret_error;
}


/* Module stuff
 */

MODULE_INFO_INIT_EASY (balancer, round_robin);

void
MODULE_INIT(round_robin) (cherokee_module_loader_t *loader)
{
}
