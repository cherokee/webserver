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

#include "common-internal.h"
#include "bogotime.h"
#include "dtm.h"
#include "util.h"

/* Global bogo time entry
 */

/* Thread safe */
time_t            cherokee_bogonow_now;
int               cherokee_bogonow_tzloc_sign;
cuint_t           cherokee_bogonow_tzloc_offset;

/* Thread unsafe */
struct tm         cherokee_bogonow_tmloc;
struct tm         cherokee_bogonow_tmgmt;
cherokee_buffer_t cherokee_bogonow_strgmt;

/* Global
 */
static cherokee_boolean_t inited = false;
#ifdef HAVE_PTHREAD
static pthread_rwlock_t   lock   = PTHREAD_RWLOCK_INITIALIZER;
#endif


/* Multi-threading support
 */

void
cherokee_bogotime_lock_read (void)
{
	CHEROKEE_RWLOCK_READER (&lock);
}

void
cherokee_bogotime_release (void)
{
	CHEROKEE_RWLOCK_UNLOCK (&lock);
}


/* Functions
 */
ret_t
cherokee_bogotime_init (void)
{
	if (inited)
		return ret_ok;

	cherokee_bogonow_now          = 0;
	cherokee_bogonow_tzloc_sign   = '+';
	cherokee_bogonow_tzloc_offset = 0;

	cherokee_buffer_init (&cherokee_bogonow_strgmt);

	inited = true;
	return ret_ok;
}

ret_t
cherokee_bogotime_free (void)
{
	if (! inited)
		return ret_ok;

	cherokee_buffer_mrproper (&cherokee_bogonow_strgmt);
	CHEROKEE_RWLOCK_DESTROY (&lock);

	inited = false;
	return ret_ok;
}


static ret_t
update_guts (void)
{
	time_t  newtime;
	size_t  szlen = 0;
	char    bufstr[DTM_SIZE_GMTTM_STR + 2];

	/* Read the time
	 */
	newtime = time (NULL);
	if (cherokee_bogonow_now >= newtime) 
		return ret_ok;

	/* Update internal variables
	 */
	cherokee_bogonow_now = newtime;

	/* Convert time to both GMT and local time struct
	 */
	cherokee_gmtime    (&newtime, &cherokee_bogonow_tmgmt);
	cherokee_localtime (&newtime, &cherokee_bogonow_tmloc);

#ifdef HAVE_STRUCT_TM_GMTOFF
	cherokee_bogonow_tzloc_sign   = cherokee_bogonow_tmloc.tm_gmtoff < 0 ? '-' : '+';
	cherokee_bogonow_tzloc_offset = abs (cherokee_bogonow_tmloc.tm_gmtoff / 3600);
#else
	cherokee_bogonow_tzloc_sign   = timezone < 0 ? '-' : '+';
	cherokee_bogonow_tzloc_offset = abs (timezone / 3600);
#endif

	cherokee_buffer_clean (&cherokee_bogonow_strgmt);

	szlen = cherokee_dtm_gmttm2str (bufstr, sizeof(bufstr), &cherokee_bogonow_tmgmt);
	cherokee_buffer_add (&cherokee_bogonow_strgmt, bufstr, szlen);

	/* NOTE: a local time string should have {+-}timezone_offset (hours)
	 *       added to date-time GMT string.
	 */
	return ret_ok;
}


ret_t
cherokee_bogotime_update (void)
{
	ret_t ret;

	CHEROKEE_RWLOCK_WRITER (&lock);
	ret = update_guts();
	CHEROKEE_RWLOCK_UNLOCK (&lock);

	return ret;
}


ret_t
cherokee_bogotime_try_update (void)
{
	ret_t ret;
	int   unlocked;

	unlocked = CHEROKEE_RWLOCK_TRYREADER (&lock);
	if (unlocked) 
		return ret_not_found;

	ret = update_guts();
	CHEROKEE_RWLOCK_UNLOCK (&lock);

	return ret;
}
