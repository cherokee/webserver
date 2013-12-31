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

#include "balancer_round_robin.h"
#include "plugin_loader.h"
#include "bogotime.h"


/* Plug-in initialization
 */
PLUGIN_INFO_BALANCER_EASIEST_INIT (round_robin);


ret_t
cherokee_balancer_round_robin_configure (cherokee_balancer_t    *balancer,
                                         cherokee_server_t      *srv,
                                         cherokee_config_node_t *conf)
{
	ret_t                            ret;
	cherokee_balancer_round_robin_t *bal_rr = BAL_RR(balancer);

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
	bal_rr->last_one = balancer->entries.next;

	return ret_ok;
}

static ret_t
reactivate_entry (cherokee_balancer_entry_t *entry)
{
	/* balancer->mutex is LOCKED
	 */
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	/* Disable
	 */
	if (entry->disabled == false)
		return ret_ok;

	entry->disabled = false;

	/* Notify
	 */
	cherokee_source_copy_name (entry->source, &tmp);
	LOG_WARNING (CHEROKEE_ERROR_BALANCER_ONLINE_SOURCE, tmp.buf);
	cherokee_buffer_mrproper (&tmp);

	return ret_ok;
}

static ret_t
dispatch (cherokee_balancer_round_robin_t *balancer,
          cherokee_connection_t           *conn,
          cherokee_source_t              **src)
{
	cherokee_balancer_entry_t *entry;
	cuint_t                    tries  = 0;
	cherokee_balancer_t       *gbal   = BAL(balancer);

	UNUSED(conn);
	CHEROKEE_MUTEX_LOCK (&balancer->mutex);

	/* Pick the next entry */
	do {
		balancer->last_one = balancer->last_one->next;
		if (balancer->last_one == &gbal->entries) {
			balancer->last_one = gbal->entries.next;
		}

		/* Is it active?  */
		entry = BAL_ENTRY(balancer->last_one);
		if (! entry->disabled)
			break;

		if (cherokee_bogonow_now >= entry->disabled_until) {
			/* Let's give this source another chance */
			reactivate_entry (entry);
			break;
		}

		tries += 1;

		/* Count how many it's checked so far */
		if (tries > gbal->entries_len) {
			LOG_WARNING_S (CHEROKEE_ERROR_BALANCER_EXHAUSTED);
			reactivate_entry (entry);
			break;
		}
	} while (true);

	/* Found */
	*src = entry->source;

	CHEROKEE_MUTEX_UNLOCK (&balancer->mutex);
	return ret_ok;
}


static ret_t
report_fail (cherokee_balancer_round_robin_t *balancer,
             cherokee_connection_t           *conn,
             cherokee_source_t               *src)
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
		entry->disabled       = true;
		entry->disabled_until = cherokee_bogonow_now + BAL_DISABLE_TIMEOUT;

		/* Notify what has happened
		 */
		cherokee_source_copy_name (entry->source, &tmp);
		LOG_WARNING (CHEROKEE_ERROR_BALANCER_OFFLINE_SOURCE, tmp.buf);
		cherokee_buffer_mrproper (&tmp);

		CHEROKEE_MUTEX_UNLOCK (&balancer->mutex);
		return ret_ok;
	}

	SHOULDNT_HAPPEN;
	ret = ret_error;

out:
	CHEROKEE_MUTEX_UNLOCK (&balancer->mutex);
	return ret;
}


ret_t
cherokee_balancer_round_robin_new (cherokee_balancer_t **bal)
{
	CHEROKEE_NEW_STRUCT (n, balancer_round_robin);

	/* Init
	 */
	cherokee_balancer_init_base (BAL(n), PLUGIN_INFO_PTR(round_robin));

	MODULE(n)->free     = (module_func_free_t) cherokee_balancer_round_robin_free;
	BAL(n)->configure   = (balancer_configure_func_t) cherokee_balancer_round_robin_configure;
	BAL(n)->dispatch    = (balancer_dispatch_func_t) dispatch;
	BAL(n)->report_fail = (balancer_report_fail_func_t) report_fail;

	/* Init properties
	 */
	n->last_one = NULL;
	CHEROKEE_MUTEX_INIT (&n->mutex, CHEROKEE_MUTEX_FAST);

	/* Return obj
	 */
	*bal = BAL(n);
	return ret_ok;
}


ret_t
cherokee_balancer_round_robin_free (cherokee_balancer_round_robin_t *balancer)
{
	CHEROKEE_MUTEX_DESTROY (&balancer->mutex);
	return ret_ok;
}
