/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Sylvain Munaut <s.munaut@whatever-company.com>
 *
 * Copyright (C) 2013 Whatever s.a.
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

#ifndef CHEROKEE_LOGGER_MULTI_H
#define CHEROKEE_LOGGER_MULTI_H

#include "common-internal.h"
#include "logger.h"
#include "virtual_server.h"
#include "plugin_loader.h"

#define LOG_MULTI_MAX_CHILDREN	16

typedef struct {
	cherokee_logger_t  logger;

	cherokee_logger_t *child[LOG_MULTI_MAX_CHILDREN];
	int                n_children;
} cherokee_logger_multi_t;

#define LOG_MULTI(x) ((cherokee_logger_multi_t *)(x))

void PLUGIN_INIT_NAME(multi) (cherokee_plugin_loader_t *loader);

ret_t cherokee_logger_multi_new              (cherokee_logger_t       **logger, cherokee_virtual_server_t *vsrv, cherokee_config_node_t *config);
ret_t cherokee_logger_multi_free             (cherokee_logger_multi_t *logger);
ret_t cherokee_logger_multi_init             (cherokee_logger_multi_t *logger);

ret_t cherokee_logger_multi_flush            (cherokee_logger_multi_t *logger);
ret_t cherokee_logger_multi_reopen           (cherokee_logger_multi_t *logger);

ret_t cherokee_logger_multi_write_access     (cherokee_logger_multi_t *logger, cherokee_connection_t *conn);

#endif /* CHEROKEE_LOGGER_MULTI_H */
