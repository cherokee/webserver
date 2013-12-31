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

#ifndef CHEROKEE_LOGGER_CUSTOM_H
#define CHEROKEE_LOGGER_CUSTOM_H

#include "common-internal.h"
#include "connection.h"
#include "template.h"
#include "logger.h"
#include "logger_writer.h"
#include "virtual_server.h"
#include "plugin_loader.h"

typedef struct {
	cherokee_logger_t         logger;
	cherokee_template_t       template_conn;
	cherokee_logger_writer_t *writer_access;
} cherokee_logger_custom_t;

#define LOG_CUSTOM(x) ((cherokee_logger_custom_t *)(x))

void PLUGIN_INIT_NAME(custom) (cherokee_plugin_loader_t *loader);

ret_t cherokee_logger_custom_new              (cherokee_logger_t       **logger, cherokee_virtual_server_t *vsrv, cherokee_config_node_t *config);
ret_t cherokee_logger_custom_free             (cherokee_logger_custom_t *logger);
ret_t cherokee_logger_custom_init             (cherokee_logger_custom_t *logger);

ret_t cherokee_logger_custom_flush            (cherokee_logger_custom_t *logger);
ret_t cherokee_logger_custom_reopen           (cherokee_logger_custom_t *logger);

ret_t cherokee_logger_custom_write_access     (cherokee_logger_custom_t *logger, cherokee_connection_t *conn);
ret_t cherokee_logger_custom_write_string     (cherokee_logger_custom_t *logger, const char *string);

#endif /* CHEROKEE_LOGGER_CUSTOM_H */
