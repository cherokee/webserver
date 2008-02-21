/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * This piece of code by:
 * 	Pablo Neira Ayuso <pneira@optimat.com>
 *      Miguel Angel Ajo Pelayo <ajo@godsmaze.org> 
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef CHEROKEE_LOGGER_W3C_H
#define CHEROKEE_LOGGER_W3C_H

#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "connection.h"
#include "logger.h"
#include "logger_writer.h"

typedef struct {
	cherokee_logger_t        logger;	
	cherokee_boolean_t       header_added;
	time_t                   now_time;
	cherokee_buffer_t        now_buf;
	cherokee_logger_writer_t writer;
} cherokee_logger_w3c_t;


ret_t cherokee_logger_w3c_new (cherokee_logger_t **logger, cherokee_config_node_t *config);
ret_t cherokee_logger_w3c_init_base (cherokee_logger_w3c_t *logger, cherokee_config_node_t *config);

/* virtual methods implementation
 */
ret_t cherokee_logger_w3c_init         (cherokee_logger_w3c_t *logger);
ret_t cherokee_logger_w3c_free         (cherokee_logger_w3c_t *logger);

ret_t cherokee_logger_w3c_flush        (cherokee_logger_w3c_t *logger);
ret_t cherokee_logger_w3c_reopen       (cherokee_logger_w3c_t *logger);

ret_t cherokee_logger_w3c_write_access (cherokee_logger_w3c_t *logger, cherokee_connection_t *conn);
ret_t cherokee_logger_w3c_write_error  (cherokee_logger_w3c_t *logger, cherokee_connection_t *conn);
ret_t cherokee_logger_w3c_write_string (cherokee_logger_w3c_t *logger, const char *string);

#endif /* CHEROKEE_LOGGER_W3C_H */
