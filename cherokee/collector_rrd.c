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
#include "collector_rrd.h"
#include "module.h"
#include "plugin_loader.h"
#include "bogotime.h"
#include "virtual_server.h"
#include "server-protected.h"
#include "util.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif


/* Plug-in initialization
 */
PLUGIN_INFO_COLLECTOR_EASIEST_INIT (rrd);

#define ELAPSE_UPDATE     60
#define WORKER_INIT_SLEEP 10
#define ENTRIES "collector,rrd"


static ret_t
srv_init (cherokee_collector_rrd_t *rrd)
{
	ret_t ret;

	/* Configuration
	 */
	cherokee_buffer_init       (&rrd->path_database);
	cherokee_buffer_add_buffer (&rrd->path_database, &rrd_connection->path_databases);
	cherokee_buffer_add_str    (&rrd->path_database, "/server.rrd");

	/* Check whether the DB exists
	 */
	ret = cherokee_rrd_connection_create_srv_db (rrd_connection);
	if (ret != ret_ok) {
		return ret_error;
	}

	return ret_ok;
}


static ret_t
update_generic (cherokee_buffer_t *params)
{
	ret_t ret;

	/* Execute
	 */
	ret = cherokee_rrd_connection_execute (rrd_connection, params);
	if (ret != ret_ok) {
		LOG_ERROR (CHEROKEE_ERROR_COLLECTOR_COMMAND_EXEC, params->buf);
		cherokee_rrd_connection_kill (rrd_connection, false);
		return ret_error;
	}

	/* Check everything was alright
	 */
	if ((params->len < 3) &&
	    (strncmp (params->buf, "OK", 2) != 0))
	{
		cherokee_rrd_connection_kill (rrd_connection, false);
		return ret_error;
	}

	return ret_ok;
}


static void
update_srv_cb (cherokee_collector_rrd_t *rrd)
{
	ret_t ret;

	/* Build the RRDtool string
	 */
	cherokee_buffer_clean        (&rrd->tmp);
	cherokee_buffer_add_str      (&rrd->tmp, "update ");
	cherokee_buffer_add_buffer   (&rrd->tmp, &rrd->path_database);
	cherokee_buffer_add_str      (&rrd->tmp, " N:");
	cherokee_buffer_add_ullong10 (&rrd->tmp, COLLECTOR(rrd)->accepts_partial);
	cherokee_buffer_add_str      (&rrd->tmp, ":");
	cherokee_buffer_add_ullong10 (&rrd->tmp, COLLECTOR(rrd)->requests_partial);
	cherokee_buffer_add_str      (&rrd->tmp, ":");
	cherokee_buffer_add_ullong10 (&rrd->tmp, COLLECTOR(rrd)->timeouts_partial);
	cherokee_buffer_add_str      (&rrd->tmp, ":");
	cherokee_buffer_add_ullong10 (&rrd->tmp, COLLECTOR_BASE(rrd)->rx_partial);
	cherokee_buffer_add_str      (&rrd->tmp, ":");
	cherokee_buffer_add_ullong10 (&rrd->tmp, COLLECTOR_BASE(rrd)->tx_partial);
	cherokee_buffer_add_str      (&rrd->tmp, "\n");

	/* Update
	 */
	ret = update_generic (&rrd->tmp);
	if (ret != ret_ok) {
		return;
	}

	/* Begin partial counting from scratch
	 */
	COLLECTOR(rrd)->accepts_partial  = 0;
	COLLECTOR(rrd)->requests_partial = 0;
	COLLECTOR(rrd)->timeouts_partial = 0;
	COLLECTOR_BASE(rrd)->rx_partial  = 0;
	COLLECTOR_BASE(rrd)->tx_partial  = 0;
}


static ret_t
srv_free (cherokee_collector_rrd_t *rrd)
{
	/* Stop the thread
	 */
	rrd->exiting            = true;
	rrd_connection->exiting = true;

	CHEROKEE_THREAD_KILL (rrd->thread, SIGINT);
	CHEROKEE_THREAD_JOIN (rrd->thread);

	CHEROKEE_MUTEX_DESTROY (&rrd->mutex);

	/* Clean up
	 */
	cherokee_buffer_mrproper (&rrd->path_database);
	cherokee_buffer_mrproper (&rrd->tmp);

	return cherokee_rrd_connection_mrproper (rrd_connection);
}


/* Virtual Servers
 */

static void
update_vsrv_cb (cherokee_collector_vsrv_rrd_t *rrd)
{
	ret_t ret;

	/* Build params
	 */
	cherokee_buffer_clean        (&rrd->tmp);
	cherokee_buffer_add_str      (&rrd->tmp, "update ");
	cherokee_buffer_add_buffer   (&rrd->tmp, &rrd->path_database);
	cherokee_buffer_add_str      (&rrd->tmp, " N:");
	cherokee_buffer_add_ullong10 (&rrd->tmp, COLLECTOR_BASE(rrd)->rx_partial);
	cherokee_buffer_add_str      (&rrd->tmp, ":");
	cherokee_buffer_add_ullong10 (&rrd->tmp, COLLECTOR_BASE(rrd)->tx_partial);
	cherokee_buffer_add_str      (&rrd->tmp, "\n");

	/* Update
	 */
	ret = update_generic (&rrd->tmp);
	if (ret != ret_ok) {
		return;
	}

	/* Begin partial counting from scratch
	 */
	COLLECTOR_BASE(rrd)->rx_partial = 0;
	COLLECTOR_BASE(rrd)->tx_partial = 0;
}


static ret_t
vsrv_free (cherokee_collector_vsrv_rrd_t *rrd)
{
	UNUSED(rrd);
	return ret_ok;
}


static ret_t
vsrv_init (cherokee_collector_vsrv_rrd_t  *rrd,
           cherokee_virtual_server_t      *vsrv)

{
	ret_t                     ret;
	cherokee_server_t        *srv     = VSERVER_SRV(vsrv);
	cherokee_collector_rrd_t *rrd_srv = COLLECTOR_RRD(srv->collector);

	/* Store a ref to the virtual server
	 */
	rrd->vsrv_ref = vsrv;

	/* Configuration
	 */
	cherokee_buffer_init           (&rrd->path_database);
	cherokee_buffer_add_buffer     (&rrd->path_database, &rrd_connection->path_databases);
	cherokee_buffer_add_str        (&rrd->path_database, "/vserver_");
	cherokee_buffer_add_buffer     (&rrd->path_database, &vsrv->name);
	cherokee_buffer_add_str        (&rrd->path_database, ".rrd");
	cherokee_buffer_replace_string (&rrd->path_database, " ", 1, "_", 1);

	/* Check whether the DB exists
	 */
	ret = cherokee_rrd_connection_create_vsrv_db (rrd_connection, &rrd->path_database);
	if (ret != ret_ok) {
		return ret_error;
	}

	/* Next iterations
	 */
	cherokee_list_add_tail_content (&rrd_srv->collectors_vsrv, rrd);
	return ret_ok;
}


static ret_t
vsrv_new (cherokee_collector_t           *collector,
          cherokee_config_node_t         *config,
          cherokee_collector_vsrv_rrd_t **collector_vsrv)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, collector_vsrv_rrd);

	UNUSED (collector);

	/* Base class initialization
	 */
	ret = cherokee_collector_vsrv_init_base (COLLECTOR_VSRV(n), config);
	if (ret != ret_ok) {
		return ret_error;
	}

	/* Virtual methods
	 */
	COLLECTOR_VSRV(n)->init = (collector_vsrv_func_init_t) vsrv_init;
	COLLECTOR_BASE(n)->free = (collector_func_free_t) vsrv_free;

	/* Default values
	 */
	n->draw_srv_traffic = true;
	cherokee_buffer_init (&n->tmp);

	/* Read configuration
	 */
	cherokee_config_node_read_bool (config, "draw_srv_traffic", &n->draw_srv_traffic);

	/* Return object
	 */
	*collector_vsrv = n;
	return ret_ok;
}


static void *
rrd_thread_worker_func (void *param)
{
	time_t                         start;
	cint_t                         elapse;
	cuint_t                        to_sleep;
	cherokee_list_t               *i;
	cherokee_collector_vsrv_rrd_t *vrrd;
	cherokee_collector_rrd_t      *rrd       = COLLECTOR_RRD(param);

	TRACE (ENTRIES, "Worker thread created.. sleeping %d secs\n", WORKER_INIT_SLEEP);
	sleep (WORKER_INIT_SLEEP);

	while (! rrd->exiting) {
		start = cherokee_bogonow_now;
		TRACE (ENTRIES, "Worker thread: Starting new iteration (now=%d)\n", start);

		/* Server
		 */
		update_srv_cb (rrd);

		/* Virtual Servers
		 */
		list_for_each (i, &rrd->collectors_vsrv) {
			vrrd = COLLECTOR_VSRV_RRD (LIST_ITEM_INFO(i));
			update_vsrv_cb (vrrd);
		}

		/* End of iteration
		 */
		elapse   = cherokee_bogonow_now - start;
		to_sleep = MAX (0, ELAPSE_UPDATE - elapse);
		TRACE (ENTRIES, "Worker thread: Finished iteration (elapse %d secs, sleeping %d secs)\n", elapse, to_sleep);

		if (to_sleep > 0) {
			sleep (to_sleep);
		}
	}

	pthread_exit (NULL);
	return NULL;
}


/* Plug-in constructor
 */

ret_t
cherokee_collector_rrd_new (cherokee_collector_rrd_t **rrd,
                            cherokee_plugin_info_t    *info,
                            cherokee_config_node_t    *config)
{
	int   re;
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, collector_rrd);

	/* Base class initialization
	 */
	ret = cherokee_collector_init_base (COLLECTOR(n), info, config);
	if (ret != ret_ok) {
		return ret_error;
	}

	/* Virtual methods
	 */
	COLLECTOR_BASE(n)->free = (collector_func_free_t) srv_free;
	COLLECTOR(n)->init      = (collector_func_init_t) srv_init;
	COLLECTOR(n)->new_vsrv  = (collector_func_new_vsrv_t) vsrv_new;

	/* Default values
	 */
	cherokee_buffer_init (&n->tmp);
	cherokee_buffer_init (&n->path_database);

	INIT_LIST_HEAD (&n->collectors_vsrv);

	/* Configuration
	 */
	cherokee_rrd_connection_get (NULL);

	ret = cherokee_rrd_connection_configure (rrd_connection, config);
	if (ret != ret_ok) {
		return ret;
	}

	/* Path to the RRD file
	 */
	cherokee_buffer_add_buffer (&n->path_database, &rrd_connection->path_databases);
	cherokee_buffer_add_str    (&n->path_database, "/server.rrd");

	/* Create the thread and mutex
	 */
	n->exiting = false;

	re = pthread_create (&n->thread, NULL, rrd_thread_worker_func, n);
	if (re != 0) {
		LOG_ERROR (CHEROKEE_ERROR_COLLECTOR_NEW_THREAD, re);
		return ret_error;
	}

	re = pthread_mutex_init (&n->mutex, NULL);
	if (re != 0) {
		LOG_ERROR (CHEROKEE_ERROR_COLLECTOR_NEW_MUTEX, re);
		return ret_error;
	}

	/* Return obj
	 */
	*rrd = n;
	return ret_ok;
}
