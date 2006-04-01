/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * This piece of code by:
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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_LOGGER_H
#define CHEROKEE_LOGGER_H

#include <cherokee/common.h>
#include <cherokee/module.h>
#include <cherokee/table.h>
#include <cherokee/buffer.h>


CHEROKEE_BEGIN_DECLS

/* Callback function definitions
 */
typedef ret_t (* logger_func_new_t)          (void **logger, cherokee_table_t *properties);
typedef ret_t (* logger_func_init_t)         (void  *logger);
typedef ret_t (* logger_func_free_t)         (void  *logger);
typedef ret_t (* logger_func_flush_t)        (void  *logger);
typedef ret_t (* logger_func_reopen_t)       (void  *logger);
typedef ret_t (* logger_func_write_access_t) (void  *logger, void *conn);
typedef ret_t (* logger_func_write_error_t)  (void  *logger, void *conn);
typedef ret_t (* logger_func_write_string_t) (void  *logger, const char *format);


typedef struct {
	/* Base
	 */
	cherokee_module_t module;

	/* Private data
	 */
	struct cherokee_logger_private *priv;

	/* Pure virtual methods
	 */
	logger_func_flush_t        flush;
	logger_func_reopen_t       reopen;
	logger_func_write_access_t write_access;
	logger_func_write_error_t  write_error;
	logger_func_write_string_t write_string;

	/* Buffer 
	 */
	cherokee_buffer_t *buffer;

} cherokee_logger_t;

#define LOGGER(x)        ((cherokee_logger_t *)(x))
#define LOGGER_BUFFER(b) (LOGGER(b)->buffer)


ret_t cherokee_logger_init_base (cherokee_logger_t *logger);

ret_t cherokee_logger_init  (cherokee_logger_t *logger);
ret_t cherokee_logger_free  (cherokee_logger_t *logger);

ret_t cherokee_logger_reopen (cherokee_logger_t *logger);
ret_t cherokee_logger_flush  (cherokee_logger_t *logger);

ret_t cherokee_logger_write_access (cherokee_logger_t *logger, void *conn);
ret_t cherokee_logger_write_error  (cherokee_logger_t *logger, void *conn);
ret_t cherokee_logger_write_string (cherokee_logger_t *logger, const char *format, ...);

ret_t cherokee_logger_set_backup_mode (cherokee_logger_t *logger, cherokee_boolean_t active);
ret_t cherokee_logger_get_backup_mode (cherokee_logger_t *logger, cherokee_boolean_t *active);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_LOGGER_H */
