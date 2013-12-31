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

#ifndef CHEROKEE_LOGGER_COMBINED_H
#define CHEROKEE_LOGGER_COMBINED_H

#include "logger_ncsa.h"
#include "plugin_loader.h"

typedef cherokee_logger_ncsa_t cherokee_logger_combined_t;

#define LOG_COMBINED(x) ((cherokee_logger_combined_t *)(x))

void PLUGIN_INIT_NAME(combined) (cherokee_plugin_loader_t *loader);

ret_t cherokee_logger_combined_new (cherokee_logger_t **logger, cherokee_virtual_server_t *vsrv, cherokee_config_node_t *config);

#endif /* CHEROKEE_LOGGER_COMBINED_H */
