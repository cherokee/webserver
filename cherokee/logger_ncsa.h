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

#ifndef CHEROKEE_LOGGER_NCSA_H
#define CHEROKEE_LOGGER_NCSA_H

#include <time.h>
#include "common-internal.h"
#include "connection.h"
#include "logger.h"
#include "logger_writer.h"
#include "virtual_server.h"
#include "plugin_loader.h"

typedef struct {
	cherokee_logger_t logger;

	cherokee_boolean_t combined;

	long               tz;
	time_t             now_time;
	cherokee_buffer_t  now_dtm;

	cherokee_buffer_t  referer;
	cherokee_buffer_t  useragent;

	cherokee_logger_writer_t *writer_access;
} cherokee_logger_ncsa_t;

#define LOG_NCSA(x) ((cherokee_logger_ncsa_t *)(x))

void PLUGIN_INIT_NAME(ncsa)          (cherokee_plugin_loader_t *loader);
ret_t cherokee_logger_ncsa_new       (cherokee_logger_t     **logger, cherokee_virtual_server_t *vsrv, cherokee_config_node_t *config);
ret_t cherokee_logger_ncsa_init_base (cherokee_logger_ncsa_t *logger, cherokee_virtual_server_t *vsrv, cherokee_config_node_t *config);
ret_t cherokee_logger_ncsa_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **props);

/* virtual methods implementation
 */
ret_t cherokee_logger_ncsa_init         (cherokee_logger_ncsa_t *logger);
ret_t cherokee_logger_ncsa_free         (cherokee_logger_ncsa_t *logger);

ret_t cherokee_logger_ncsa_flush        (cherokee_logger_ncsa_t *logger);
ret_t cherokee_logger_ncsa_reopen       (cherokee_logger_ncsa_t *logger);
ret_t cherokee_logger_ncsa_write_access (cherokee_logger_ncsa_t *logger, cherokee_connection_t *conn);

#endif /* CHEROKEE_LOGGER_NCSA_H */
