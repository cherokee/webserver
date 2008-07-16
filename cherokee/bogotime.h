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

#ifndef CHEROKEE_BOGOTIME_H
#define CHEROKEE_BOGOTIME_H

#include <cherokee/common.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else 
# include <time.h>
#endif

#include <cherokee/buffer.h>

/* Global bogonow variables
 */

/* Thread safe */
extern time_t             cherokee_bogonow_now;
extern int                cherokee_bogonow_tzloc_sign;
extern cuint_t            cherokee_bogonow_tzloc_offset;

/* Thread unsafe */
extern struct tm          cherokee_bogonow_tmloc;
extern struct tm          cherokee_bogonow_tmgmt;
extern cherokee_buffer_t  cherokee_bogonow_strgmt;

/* Functions
 */
ret_t cherokee_bogotime_init       (void);
ret_t cherokee_bogotime_free       (void);

ret_t cherokee_bogotime_update     (void);
ret_t cherokee_bogotime_try_update (void);

/* Multi-threading */
void  cherokee_bogotime_lock_read  (void);
void  cherokee_bogotime_release    (void);

#endif /* CHEROKEE_BOGOTIME_H */
