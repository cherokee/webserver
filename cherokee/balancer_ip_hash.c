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

#include "common-internal.h"

#include "balancer_ip_hash.h"
#include "plugin_loader.h"
#include "bogotime.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "balancer,iphash"


/* Plug-in initialization
 */
PLUGIN_INFO_BALANCER_EASIEST_INIT (ip_hash);


ret_t
cherokee_balancer_ip_hash_configure (cherokee_balancer_t    *balancer,
                                     cherokee_server_t      *srv,
                                     cherokee_config_node_t *conf)
{
	ret_t                        ret;
	cherokee_list_t             *i;
	cherokee_balancer_ip_hash_t *bal_ip = BAL_IP_HASH(balancer);

	/* Configure the generic balancer
	 */
	ret = cherokee_balancer_configure_base (balancer, srv, conf);
	if (ret != ret_ok)
		return ret;

	/* Sanity check
	 */
	if (balancer->entries_len <= 0) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_BALANCER_EMPTY);
		return ret_error;
	}

	/* Set the current source pointer
	 */
	bal_ip->last_one = balancer->entries.next;

	/* Count active
	 */
	list_for_each (i, &BAL(balancer)->entries) {
		if (! BAL_ENTRY(i)->disabled) {
			bal_ip->n_active += 1;
		}
	}

	return ret_ok;
}


static ret_t
reactivate_entry (cherokee_balancer_ip_hash_t *balancer,
                  cherokee_balancer_entry_t   *entry)
{
	/* balancer->mutex is LOCKED
	 */
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	/* Disable
	 */
	if (entry->disabled == false)
		return ret_ok;

	balancer->n_active += 1;
	entry->disabled = false;

	/* Notify
	 */
	cherokee_source_copy_name (entry->source, &tmp);
	LOG_WARNING (CHEROKEE_ERROR_BALANCER_IP_REACTIVE, tmp.buf, balancer->n_active);
	cherokee_buffer_mrproper (&tmp);

	return ret_ok;
}


static ret_t
report_fail (cherokee_balancer_ip_hash_t *balancer,
             cherokee_connection_t       *conn,
             cherokee_source_t           *src)
{
	ret_t                      ret;
	cherokee_list_t           *i;
	cherokee_balancer_entry_t *entry;
	cherokee_buffer_t          tmp    = CHEROKEE_BUF_INIT;

	UNUSED(conn);

	CHEROKEE_MUTEX_LOCK (&balancer->mutex);

	list_for_each (i, &BAL(balancer)->entries) {
		entry = BAL_ENTRY(i);

		/* Find the right source
		 */
		if (entry->source != src)
			continue;

		if (entry->disabled) {
			ret = ret_ok;
			goto out;
		}

		/* Disable the source
		 */
		balancer->n_active -= 1;

		entry->disabled       = true;
		entry->disabled_until = cherokee_bogonow_now + BAL_DISABLE_TIMEOUT;

		/* Notify what has happened
		 */
		cherokee_source_copy_name (entry->source, &tmp);
		LOG_WARNING (CHEROKEE_ERROR_BALANCER_IP_DISABLE, tmp.buf, balancer->n_active);
		cherokee_buffer_mrproper (&tmp);

		CHEROKEE_MUTEX_UNLOCK (&balancer->mutex);
		return ret_ok;
	}

	ret = ret_error;
	SHOULDNT_HAPPEN;

out:
	CHEROKEE_MUTEX_UNLOCK (&balancer->mutex);
	return ret;
}


static ret_t
dispatch (cherokee_balancer_ip_hash_t  *balancer,
          cherokee_connection_t        *conn,
          cherokee_source_t           **src)
{
	cint_t                     n;
	cint_t                     ip_len;
	char                      *ip;
	cherokee_list_t           *i;
	cherokee_balancer_entry_t *entry  = NULL;
	culong_t                   hash   = 0;
	cherokee_socket_t         *socket = &conn->socket;

	CHEROKEE_MUTEX_LOCK (&balancer->mutex);

	/* Hash(ip)
	 */
#ifdef HAVE_IPV6
	if (SOCKET_AF(socket) == AF_INET6) {
		ip     = (char *)&SOCKET_ADDRESS_IPv6(socket);
		ip_len = 16;
	} else
#endif
	{
		ip     = (char *)&SOCKET_ADDRESS_IPv4(socket);
		ip_len = 4;
	}

	for (n=0; n < ip_len; n++) {
		hash += ip[n];
	}

	TRACE(ENTRIES, "IP len=%d hash=%u active_server=%d\n",
	      ip_len, hash, balancer->n_active);

	/* Select a back-end
	 */
	if (unlikely (balancer->n_active <= 0)) {
		LOG_WARNING_S (CHEROKEE_ERROR_BALANCER_IP_EXHAUSTED);
		reactivate_entry (balancer, BAL_ENTRY(balancer->last_one));

		balancer->last_one = list_next_circular (&BAL(balancer)->entries,
		                                         balancer->last_one);
	}

	n = (hash % balancer->n_active);
	TRACE(ENTRIES, "Chosen active server number %d\n", n);

	/* Pick the entry
	 */
	list_for_each (i, &BAL(balancer)->entries) {
		entry = BAL_ENTRY(i);

		if (entry->disabled) {
			if (cherokee_bogonow_now >= entry->disabled_until) {
				/* Let's give this source another chance */
				reactivate_entry (balancer, entry);
			}
			continue;
		}

		if (n == 0)
			break;
		n--;
	}

	/* Found */
	if (unlikely (entry == NULL))
		goto error;

	*src = entry->source;
	CHEROKEE_MUTEX_UNLOCK (&balancer->mutex);
	return ret_ok;

error:
	*src = NULL;
	CHEROKEE_MUTEX_UNLOCK (&balancer->mutex);
	return ret_error;
}


ret_t
cherokee_balancer_ip_hash_new (cherokee_balancer_t **bal)
{
	CHEROKEE_NEW_STRUCT (n, balancer_ip_hash);

	/* Init
	 */
	cherokee_balancer_init_base (BAL(n), PLUGIN_INFO_PTR(ip_hash));

	MODULE(n)->free     = (module_func_free_t) cherokee_balancer_ip_hash_free;
	BAL(n)->configure   = (balancer_configure_func_t) cherokee_balancer_ip_hash_configure;
	BAL(n)->dispatch    = (balancer_dispatch_func_t) dispatch;
	BAL(n)->report_fail = (balancer_report_fail_func_t) report_fail;

	/* Init properties
	 */
	n->last_one = NULL;
	n->n_active = 0;

	CHEROKEE_MUTEX_INIT (&n->mutex, CHEROKEE_MUTEX_FAST);

	/* Return obj
	 */
	*bal = BAL(n);
	return ret_ok;
}


ret_t
cherokee_balancer_ip_hash_free (cherokee_balancer_ip_hash_t *balancer)
{
	CHEROKEE_MUTEX_DESTROY (&balancer->mutex);
	return ret_ok;
}
