/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
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

#ifndef CHEROKEE_LOGGER_TABLE_H
#define CHEROKEE_LOGGER_TABLE_H

#include "common.h"
#include "table.h"
#include "table-protected.h"
#include "logger.h"
#include "module.h"

typedef struct {
	   cherokee_table_t table;
} cherokee_logger_table_t;

#define LTABLE(x) ((cherokee_logger_table_t *)(x))

/* Logger table methods
 */
ret_t cherokee_logger_table_new   (cherokee_logger_table_t **et);
ret_t cherokee_logger_table_free  (cherokee_logger_table_t  *et);
ret_t cherokee_logger_table_clean (cherokee_logger_table_t  *et);

ret_t cherokee_logger_table_get        (cherokee_logger_table_t  *et, char *logger, cherokee_module_info_t **info);
ret_t cherokee_logger_table_new_logger (cherokee_logger_table_t  *et, char *logger, cherokee_module_info_t  *info, cherokee_table_t *props, cherokee_logger_t **new_logger);


#endif /* CHEROKEE_LOGGER_TABLE_H */
