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
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_LOGGER_NCSA_H
#define CHEROKEE_LOGGER_NCSA_H

#include "common-internal.h"
#include "connection.h"
#include "logger.h"


typedef struct {
	cherokee_logger_t logger;

	cherokee_boolean_t combined;

	char *accesslog_filename;
	char *errorlog_filename;

	FILE *accesslog_fd;
	FILE *errorlog_fd;

} cherokee_logger_ncsa_t;


ret_t cherokee_logger_ncsa_new       (cherokee_logger_t     **logger, cherokee_table_t *properties);
ret_t cherokee_logger_ncsa_init_base (cherokee_logger_ncsa_t *logger, cherokee_table_t *properties);

/* virtual methods implementation
 */
ret_t cherokee_logger_ncsa_init         (cherokee_logger_ncsa_t *logger);
ret_t cherokee_logger_ncsa_free         (cherokee_logger_ncsa_t *logger);

ret_t cherokee_logger_ncsa_flush        (cherokee_logger_ncsa_t *logger);
ret_t cherokee_logger_ncsa_reopen       (cherokee_logger_ncsa_t *logger);

ret_t cherokee_logger_ncsa_write_access (cherokee_logger_ncsa_t *logger, cherokee_connection_t *conn);
ret_t cherokee_logger_ncsa_write_error  (cherokee_logger_ncsa_t *logger, cherokee_connection_t *conn);
ret_t cherokee_logger_ncsa_write_string (cherokee_logger_ncsa_t *logger, const char *string);

#endif /* CHEROKEE_LOGGER_NCSA_H */
