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


#ifndef CHEROKEE_COMMON_H
#define CHEROKEE_COMMON_H

#include <cherokee/macros.h>


CHEROKEE_BEGIN_DECLS

typedef enum {
	false = 0,
	true  = 1
} cherokee_boolean_t;

typedef enum {
	ret_no_sys          = -4,
	ret_nomem           = -3,
	ret_deny            = -2,
	ret_error           = -1,
	ret_ok              =  0,
	ret_eof             =  1,
	ret_eof_have_data   =  2,
	ret_not_found       =  3,
	ret_file_not_found  =  4,
	ret_eagain          =  5,
	ret_ok_and_sent     =  6
} ret_t;

typedef unsigned int   crc_t;

typedef int                 cint_t;
typedef char                cchar_t;
typedef short               cshort_t;
typedef long                clong_t;
typedef long long           cllong_t;

typedef unsigned int        cuint_t;
typedef unsigned char       cuchar_t;
typedef unsigned short      cushort_t;
typedef unsigned long       culong_t;
typedef unsigned long long  cullong_t;

typedef void (*cherokee_func_free_t) (void *);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_COMMON_H */
