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


#ifndef CHEROKEE_NULLABLE_H
#define CHEROKEE_NULLABLE_H

CHEROKEE_BEGIN_DECLS

typedef int cherokee_null_bool_t;
typedef int cherokee_null_int_t;

#define NULLB_NULL       0xFC
#define NULLB_TO_BOOL(x) ((x) & 0x3)
#define NULLB_IS_NULL(x) ((x) & NULLB_NULL)
#define NULLB_TO_STR(x)  (NULLB_IS_NULL(x) ? "NULL" : ((x) ? "True" : "False"))

#define NULLI_NULL       INT_MAX
#define NULLI_IS_NULL(x) ((x) == NULLI_NULL)

CHEROKEE_END_DECLS

#endif /* CHEROKEE_NULLABLE_H */
