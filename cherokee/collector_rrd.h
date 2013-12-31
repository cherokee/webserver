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

#ifndef CHEROKEE_COLLECTOR_RRD_H
#define CHEROKEE_COLLECTOR_RRD_H

#include "common-internal.h"
#include "collector.h"
#include "rrd_tools.h"
#include "plugin_loader.h"

/* Enums
 */
typedef enum {
	collector_rrd_vserver_traffic,
	collector_rrd_server_timeouts,
	collector_rrd_server_accepts,
	collector_rrd_server_requests
} cherokee_collector_rrd_graphs_t;


/* Data types
 */
typedef struct {
	cherokee_collector_t      collector;

	/* Configuration*/
	cherokee_buffer_t         path_database;

	/* Internals */
	cherokee_buffer_t         tmp;

	/* Asynchronous */
	pthread_t                 thread;
	pthread_mutex_t           mutex;
	cherokee_boolean_t        exiting;

	cherokee_list_t           collectors_vsrv;
} cherokee_collector_rrd_t;


typedef struct {
	cherokee_collector_vsrv_t  collector;

	/* Configuration*/
	cherokee_buffer_t          path_database;
	cherokee_boolean_t         draw_srv_traffic;

	/* Internals */
	void                      *vsrv_ref;
	cherokee_buffer_t          tmp;

	/* Asynchronous */
	time_t                     next_render;
	time_t                     next_update;
} cherokee_collector_vsrv_rrd_t;

#define COLLECTOR_RRD(c)          ((cherokee_collector_rrd_t *)(c))
#define COLLECTOR_VSRV_RRD(c)     ((cherokee_collector_vsrv_rrd_t *)(c))
#define COLLECTOR_VSRV_RRD_SRV(c) (COLLECTOR_RRD(VSERVER_SRV(COLLECTOR_VSRV_RRD(c)->vsrv_ref)->collector))

void PLUGIN_INIT_NAME(rrd) (cherokee_plugin_loader_t *loader);

/* Public methods
 */
ret_t cherokee_collector_rrd_new (cherokee_collector_rrd_t **rrd,
                                  cherokee_plugin_info_t    *info,
                                  cherokee_config_node_t    *config);

#endif /* CHEROKEE_COLLECTOR_RRD_H */
