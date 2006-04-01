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

#include "logger_table.h"
#include "common-internal.h"

ret_t 
cherokee_logger_table_new  (cherokee_logger_table_t **lt)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n, logger_table);
	
	/* This table is going to store references to the modules
	 * that the server should load before add a new logger to
	 * this table.  Eg:  "ncsa" -> pointer to ncsa module.
	 */
	ret = cherokee_table_init (&n->table);
	if (unlikely(ret < ret_ok)) return ret;
	
	/* Return the object
	 */
	*lt = n;
	
	return ret_ok;
}


ret_t 
cherokee_logger_table_free (cherokee_logger_table_t *et)
{
	return cherokee_table_free (&et->table);
}


ret_t
cherokee_logger_table_get (cherokee_logger_table_t *et, char *logger, cherokee_module_info_t **info) 
{
	ret_t ret;

	ret = cherokee_table_get (&et->table, logger, (void **)info);
	if (unlikely(ret < ret_ok)) return ret;

	return ret_ok;
}


ret_t 
cherokee_logger_table_new_logger (cherokee_logger_table_t *et, char *logger, cherokee_module_info_t *info, cherokee_table_t *props, cherokee_logger_t **new_logger)
{
	ret_t ret;
	logger_func_new_t new_func;

	ret = cherokee_logger_table_get (et, logger, &info);
	if (unlikely(ret != ret_ok)) return ret;

	new_func = (logger_func_new_t) info->new_func;
	ret = new_func ((void **)new_logger, props);
	if (unlikely(ret != ret_ok)) return ret;

	ret = cherokee_logger_init (*new_logger);
	return ret;
}
