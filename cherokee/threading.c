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

#include "common-internal.h"
#include "threading.h"

#ifdef HAVE_PTHREAD
# ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
pthread_mutexattr_t cherokee_mutexattr_fast;
# endif
# ifdef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
pthread_mutexattr_t cherokee_mutexattr_errorcheck;
# endif
#endif

ret_t
cherokee_threading_init (void)
{
#ifdef HAVE_PTHREAD
# ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
	pthread_mutexattr_init (&cherokee_mutexattr_fast);
	pthread_mutexattr_setkind_np (&cherokee_mutexattr_fast,
				      PTHREAD_MUTEX_ADAPTIVE_NP);
# endif

# ifdef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
	pthread_mutexattr_init (&cherokee_mutexattr_errorcheck);
	pthread_mutexattr_setkind_np (&cherokee_mutexattr_errorcheck,
				      PTHREAD_MUTEX_ERRORCHECK_NP);
# endif
#endif

	return ret_ok;
}


ret_t
cherokee_threading_free (void)
{
#ifdef HAVE_PTHREAD
# ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
	pthread_mutexattr_destroy (&cherokee_mutexattr_fast);
# endif

# ifdef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
	pthread_mutexattr_destroy (&cherokee_mutexattr_errorcheck);
# endif
#endif

	return ret_ok;
}

