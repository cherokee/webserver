/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_DTM_H
#define CHEROKEE_DTM_H

#include <cherokee/common.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


CHEROKEE_BEGIN_DECLS

#define DTM_TIME_EVAL		((time_t) -1)
#define DTM_LEN_GMTTM_STR	29
#define DTM_SIZE_GMTTM_STR	(DTM_LEN_GMTTM_STR + 1)

const
char  *cherokee_dtm_wday_name (int idxName);
const
char  *cherokee_dtm_month_name(int idxName);

time_t cherokee_dtm_str2time  (char *cstr);
size_t cherokee_dtm_gmttm2str (char *bufstr, size_t bufsize, struct tm *ptm);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_DTM_H */
