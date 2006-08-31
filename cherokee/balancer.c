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
#include "balancer.h"


#define DEFAULT_HOSTS_ALLOCATION 5


ret_t 
cherokee_balancer_init_base (cherokee_balancer_t *balancer)
{
	/* Init the base class
	 */
	cherokee_module_init_base (MODULE(balancer));

	/* Virtual methods
	 */
	balancer->dispatch   = NULL;
	
	/* Hosts
	 */
	balancer->hosts_len  = 0;
	balancer->hosts_size = 0;
	balancer->hosts      = NULL;

	return ret_ok;
}


ret_t 
cherokee_balancer_mrproper (cherokee_balancer_t *balancer)
{
	if (balancer->hosts != NULL) {
		free (balancer->hosts);
	}

	return ret_ok;
}


static ret_t
alloc_more_hosts (cherokee_balancer_t *balancer)
{
	size_t size;

	if (balancer->hosts == NULL) {
		size = DEFAULT_HOSTS_ALLOCATION * sizeof(cherokee_balancer_host_t *);
		balancer->hosts = (cherokee_balancer_host_t **) malloc (size);
	} else {
		size = (balancer->hosts_size + DEFAULT_HOSTS_ALLOCATION ) * sizeof(cherokee_balancer_host_t *);
		balancer->hosts = (cherokee_balancer_host_t **) realloc (balancer->hosts, size);
	}
	
	if (balancer->hosts == NULL) 
		return ret_nomem;
	
	memset (balancer->hosts + balancer->hosts_len, 0, DEFAULT_HOSTS_ALLOCATION);

	balancer->hosts_size += DEFAULT_HOSTS_ALLOCATION;
	return ret_ok;
}


ret_t 
cherokee_balancer_add_host (cherokee_balancer_t *balancer, cherokee_balancer_host_t *host)
{
	ret_t ret;

	if (balancer->hosts_len >= balancer->hosts_size) {
		ret = alloc_more_hosts (balancer);
		if (ret != ret_ok) return ret;
	}

	balancer->hosts[balancer->hosts_len] = host;
	balancer->hosts_len++;

	return ret_ok;
}


ret_t 
cherokee_balancer_dispatch (cherokee_balancer_t *balancer, cherokee_connection_t *conn, cherokee_balancer_host_t **host)
{
	if (balancer->dispatch == NULL)
		return ret_error;

	return balancer->dispatch (balancer, conn, host);
}


ret_t 
cherokee_balancer_free (cherokee_balancer_t *bal)
{
	ret_t              ret;
	cherokee_module_t *module = MODULE(bal);

	/* Sanity check
	 */
	if (module->free == NULL) {
		return ret_error;
	}
	
	/* Call the virtual method implementation
	 */
	ret = MODULE(bal)->free (bal); 
	if (unlikely (ret < ret_ok)) return ret;
	
	/* Free the rest
	 */
	cherokee_balancer_mrproper(bal);

	free (bal);
	return ret_ok;
}
