/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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

typedef struct {
	cherokee_collector_t collector;

	/* Configuration*/
	cherokee_buffer_t    path_rrdtool;
	cherokee_buffer_t    database_dir;
	cherokee_buffer_t    path_database;
	int                  render_elapse;

	/* Communication */
	int                  write_fd;
	int                  read_fd;
	pid_t                pid;

	/* Internals */
	cherokee_buffer_t    tmp;
} cherokee_collector_rrd_t;

typedef struct {
	cherokee_collector_vsrv_t  collector;

	/* Configuration*/
	cherokee_buffer_t          path_database;
	cherokee_boolean_t         draw_srv_traffic;

	/* Internals */
	void                      *vsrv_ref;
	cherokee_buffer_t          tmp;
} cherokee_collector_vsrv_rrd_t;

#define COLLECTOR_RRD(c)          ((cherokee_collector_rrd_t *)(c))
#define COLLECTOR_VSRV_RRD(c)     ((cherokee_collector_vsrv_rrd_t *)(c))
#define COLLECTOR_VSRV_RRD_SRV(c) (COLLECTOR_RRD(VSERVER_SRV(COLLECTOR_VSRV_RRD(c)->vsrv_ref)->collector))

ret_t cherokee_collector_rrd_new (cherokee_collector_rrd_t **rrd,
				  cherokee_plugin_info_t    *info,
				  cherokee_config_node_t    *config);

#endif /* CHEROKEE_COLLECTOR_RRD_H */
