/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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

typedef enum {
	cherokee_err_warning,
	cherokee_err_error,
	cherokee_err_critical
} cherokee_error_type_t;

#ifdef __GNUC__
# define LOG_WARNING(fmt,arg...)   cherokee_error_log(cherokee_err_warning,  "%s:%d - "fmt, __FILE__, __LINE__, ##arg)
# define LOG_ERROR(fmt,arg...)     cherokee_error_log(cherokee_err_error,    "%s:%d - "fmt, __FILE__, __LINE__, ##arg)
# define LOG_CRITICAL(fmt,arg...)  cherokee_error_log(cherokee_err_critical, "%s:%d - "fmt, __FILE__, __LINE__, ##arg)
# define LOG_ERRNO(t,e,fmt,arg...) cherokee_error_errno_log(e, t, "%s:%d - "fmt, __FILE__, __LINE__, ##arg)
#else
# define LOG_WARNING(t,fmt,...)   cherokee_error_log(cherokee_err_warning,  "%s:%d - "fmt, __FILE__, __LINE__, __VA_ARGS__)
# define LOG_ERROR(t,fmt,...)     cherokee_error_log(cherokee_err_error,    "%s:%d - "fmt, __FILE__, __LINE__, __VA_ARGS__)
# define LOG_CRITIAL(t,fmt,...)   cherokee_error_log(cherokee_err_critical, "%s:%d - "fmt, __FILE__, __LINE__, __VA_ARGS__)
# define LOG_ERRNO(t,e,fmt,...)   cherokee_error_errno_log(e, t, "%s:%d - "fmt, __FILE__, __LINE__, __VA_ARGS__)
#endif

#define LOG_WARNING_S(str)   LOG_WARNING("%s", str)
#define LOG_ERROR_S(str)     LOG_ERROR("%s", str)
#define LOG_CRITICAL_S(str)  LOG_CRITICAL("%s", str)
#define LOG_ERRNO_S(t,e,str) LOG_ERRNO(t,e,"%s",str)

ret_t cherokee_error_log         (cherokee_error_type_t type, const char *format, ...);
ret_t cherokee_error_errno_log   (int error, cherokee_error_type_t type, const char *format, ...);
ret_t cherokee_error_log_set_log (cherokee_logger_t *logger);


#endif /* CHEROKEE_ERROR_LOG_H */
