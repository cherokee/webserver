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

#include "balancer_failover.h"
#include "plugin_loader.h"
#include "bogotime.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "balancer,iphash"


/* Plug-in initialization
 */
PLUGIN_INFO_BALANCER_EASIEST_INIT (failover);


ret_t
cherokee_balancer_failover_configure (cherokee_balancer_t    *balancer,
                                      cherokee_server_t      *srv,
                                      cherokee_config_node_t *conf)
{
	ret_t ret;

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

	return ret_ok;
}


static ret_t
reactivate_entry_guts (cherokee_balancer_failover_t *balancer,
                       cherokee_balancer_entry_t    *entry)
{
	/* balancer->mutex is LOCKED
	 */
	UNUSED (balancer);

	/* Disable
	 */
	if (entry->disabled == false)
		return ret_ok;

	entry->disabled = false;
	return ret_ok;
}


static ret_t
reactivate_entry (cherokee_balancer_failover_t *balancer,
                  cherokee_balancer_entry_t    *entry)
{
	/* balancer->mutex is LOCKED
	 */
	ret_t             ret;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	/* Reactivate
	 */
	ret = reactivate_entry_guts (balancer, entry);

	/* Notify
	 */
	cherokee_source_copy_name (entry->source, &tmp);
	LOG_WARNING (CHEROKEE_ERROR_BALANCER_FAILOVER_REACTIVE, tmp.buf);
	cherokee_buffer_mrproper (&tmp);

	return ret;
}

static ret_t
reactivate_all_entries (cherokee_balancer_failover_t *balancer)
{
	cherokee_list_t *i;

	list_for_each (i, &BAL(balancer)->entries) {
		reactivate_entry_guts (balancer, BAL_ENTRY(i));
	}

	LOG_WARNING_S (CHEROKEE_ERROR_BALANCER_FAILOVER_ENABLE_ALL);
	return ret_ok;
}


static ret_t
report_fail (cherokee_balancer_failover_t *balancer,
             cherokee_connection_t        *conn,
             cherokee_source_t            *src)
{
	ret_t                      ret;
	cherokee_list_t           *i;
	cherokee_balancer_entry_t *entry;
	cherokee_buffer_t          tmp   = CHEROKEE_BUF_INIT;

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
		LOG_WARNING (CHEROKEE_ERROR_BALANCER_FAILOVER_DISABLE, tmp.buf);
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
dispatch (cherokee_balancer_failover_t  *balancer,
          cherokee_connection_t         *conn,
          cherokee_source_t            **src)
{
	cherokee_list_t           *i;
	cherokee_balancer_entry_t *entry = NULL;
	cherokee_balancer_t       *gbal  = BAL(balancer);

	UNUSED(conn);
	CHEROKEE_MUTEX_LOCK (&balancer->mutex);

	/* Pick the first available source
	 */
	list_for_each (i, &gbal->entries) {
		entry = BAL_ENTRY(i);

		/* Active */
		if (! entry->disabled) {
			break;
		}

		/* Reactive? */
		if (cherokee_bogonow_now >= entry->disabled_until) {
			reactivate_entry (balancer, entry);
			break;
		}

		entry = NULL;
	}

	/* No source, reactive all, and take first
	 */
	if (! entry) {
		reactivate_all_entries (balancer);
		entry = BAL_ENTRY(gbal->entries.next);
	}

	/* Return source
	 */
	*src = entry->source;

	CHEROKEE_MUTEX_UNLOCK (&balancer->mutex);
	return ret_ok;
}


ret_t
cherokee_balancer_failover_new (cherokee_balancer_t **bal)
{
	CHEROKEE_NEW_STRUCT (n, balancer_failover);

	/* Init
	 */
	cherokee_balancer_init_base (BAL(n), PLUGIN_INFO_PTR(failover));

	MODULE(n)->free     = (module_func_free_t) cherokee_balancer_failover_free;
	BAL(n)->configure   = (balancer_configure_func_t) cherokee_balancer_failover_configure;
	BAL(n)->dispatch    = (balancer_dispatch_func_t) dispatch;
	BAL(n)->report_fail = (balancer_report_fail_func_t) report_fail;

	/* Init properties
	 */
	CHEROKEE_MUTEX_INIT (&n->mutex, CHEROKEE_MUTEX_FAST);

	/* Return obj
	 */
	*bal = BAL(n);
	return ret_ok;
}


ret_t
cherokee_balancer_failover_free (cherokee_balancer_failover_t *balancer)
{
	CHEROKEE_MUTEX_DESTROY (&balancer->mutex);
	return ret_ok;
}
