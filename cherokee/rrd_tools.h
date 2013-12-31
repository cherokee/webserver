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

#ifndef CHEROKEE_RRD_TOOLS_H
#define CHEROKEE_RRD_TOOLS_H

#include "buffer.h"
#include "virtual_server.h"

/* Data model
 */
typedef struct {
	/* Configuration */
	cherokee_buffer_t  path_rrdtool;
	cherokee_buffer_t  path_databases;
	cherokee_buffer_t  path_img_cache;

	/* Connection */
	int                write_fd;
	int                read_fd;
	pid_t              pid;
	cherokee_boolean_t exiting;
	cherokee_boolean_t disabled;

	/* Threading */
	CHEROKEE_MUTEX_T  (mutex);

	/* Misc */
	cherokee_buffer_t  tmp;
} cherokee_rrd_connection_t;

typedef struct {
	const char    *interval;
	const char    *description;
	const cuint_t  secs_per_pixel;
} cherokee_collector_rrd_interval_t;


/* Globals
 */
extern cherokee_collector_rrd_interval_t  cherokee_rrd_intervals[];
extern cherokee_rrd_connection_t         *rrd_connection;


/* Methods
 */
ret_t cherokee_rrd_connection_get            (cherokee_rrd_connection_t **rrd_conn);
ret_t cherokee_rrd_connection_configure      (cherokee_rrd_connection_t  *rrd_conn, cherokee_config_node_t *config);
ret_t cherokee_rrd_connection_mrproper       (cherokee_rrd_connection_t  *rrd_conn);

ret_t cherokee_rrd_connection_create_srv_db  (cherokee_rrd_connection_t  *rrd_conn);
ret_t cherokee_rrd_connection_create_vsrv_db (cherokee_rrd_connection_t  *rrd_conn, cherokee_buffer_t *dbpath);

ret_t cherokee_rrd_connection_spawn          (cherokee_rrd_connection_t  *rrd_conn);
ret_t cherokee_rrd_connection_kill           (cherokee_rrd_connection_t  *rrd_conn, cherokee_boolean_t do_kill);
ret_t cherokee_rrd_connection_execute        (cherokee_rrd_connection_t  *rrd_conn, cherokee_buffer_t *buf);

#endif /* CHEROKEE_RRD_TOOLS_H */
