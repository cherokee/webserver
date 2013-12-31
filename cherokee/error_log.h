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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_ERROR_LOG_H
#define CHEROKEE_ERROR_LOG_H

#include <cherokee/common.h>
#include <cherokee/logger.h>
#include <cherokee/errors_defs.h>

typedef enum {
	cherokee_err_warning,
	cherokee_err_error,
	cherokee_err_critical
} cherokee_error_type_t;

typedef struct {
	int         id;
	const char *title;
	const char *description;
	const char *admin_url;
	const char *debug;
	int         show_backtrace;
} cherokee_error_t;

#define CHEROKEE_ERROR(x) ((cherokee_error_t *)(x))


#if defined(__GNUC__) || ( defined(__SUNPRO_C) && __SUNPRO_C > 0x590 )
# define LOG_WARNING(e_num,arg...)     cherokee_error_log (cherokee_err_warning,  __FILE__, __LINE__, e_num, ##arg)
# define LOG_WARNING_S(e_num)          cherokee_error_log (cherokee_err_warning,  __FILE__, __LINE__, e_num)
# define LOG_ERROR(e_num,arg...)       cherokee_error_log (cherokee_err_error,    __FILE__, __LINE__, e_num, ##arg)
# define LOG_ERROR_S(e_num)            cherokee_error_log (cherokee_err_error,    __FILE__, __LINE__, e_num)
# define LOG_CRITICAL(e_num,arg...)    cherokee_error_log (cherokee_err_critical, __FILE__, __LINE__, e_num, ##arg)
# define LOG_CRITICAL_S(e_num)         cherokee_error_log (cherokee_err_critical, __FILE__, __LINE__, e_num)
# define LOG_ERRNO(syserror,type,e_num,arg...) cherokee_error_errno_log (syserror, type, __FILE__, __LINE__, e_num, ##arg)
# define LOG_ERRNO_S(syserror,type,e_num)      cherokee_error_errno_log (syserror, type, __FILE__, __LINE__, e_num)
#else
# define LOG_WARNING(e_num,arg...)     cherokee_error_log (cherokee_err_warning,  __FILE__, __LINE__, e_num, __VA_ARGS__)
# define LOG_WARNING_S(e_num)          cherokee_error_log (cherokee_err_warning,  __FILE__, __LINE__, e_num)
# define LOG_ERROR(e_num,arg...)       cherokee_error_log (cherokee_err_error,    __FILE__, __LINE__, e_num, __VA_ARGS__)
# define LOG_ERROR_S(e_num)            cherokee_error_log (cherokee_err_error,    __FILE__, __LINE__, e_num)
# define LOG_CRITICAL(e_num,arg...)    cherokee_error_log (cherokee_err_critical, __FILE__, __LINE__, e_num, __VA_ARGS__)
# define LOG_CRITICAL_S(e_num)         cherokee_error_log (cherokee_err_critical, __FILE__, __LINE__, e_num)
# define LOG_ERRNO(syserror,type,e_num,arg...) cherokee_error_errno_log (syserror, type, __FILE__, __LINE__, e_num, __VA_ARGS__)
# define LOG_ERRNO_S(syserror,type,e_num)      cherokee_error_errno_log (syserror, type, __FILE__, __LINE__, e_num)
#endif

ret_t cherokee_error_log         (cherokee_error_type_t  type,
                                  const char            *filename,
                                  int                    line,
                                  int                    error_num, ...);

ret_t cherokee_error_errno_log   (int                    errnumber,
                                  cherokee_error_type_t  type,
                                  const char            *filename,
                                  int                    line,
                                  int                    error_num, ...);

ret_t cherokee_error_set_default (cherokee_logger_writer_t *writer);

#endif /* CHEROKEE_ERROR_LOG_H */
