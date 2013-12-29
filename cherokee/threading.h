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

#ifndef CHEROKEE_THREADING_H
#define CHEROKEE_THREADING_H

#include "common.h"

/* Global if */
#ifdef HAVE_PTHREAD

# include <pthread.h>

# if defined(HAVE_PTHREAD_MUTEXATTR_SETTYPE) || defined(HAVE_PTHREAD_MUTEXATTR_SETKIND_NP)
extern pthread_mutexattr_t cherokee_mutexattr_fast;
extern pthread_mutexattr_t cherokee_mutexattr_errorcheck;
# endif

# if defined(HAVE_PTHREAD_MUTEXATTR_SETTYPE) || defined(HAVE_PTHREAD_MUTEXATTR_SETKIND_NP)
#  define CHEROKEE_MUTEX_FAST (&cherokee_mutexattr_fast)
#  define CHEROKEE_MUTEX_ERRORCHECK (&cherokee_mutexattr_errorcheck)
# else
#  define CHEROKEE_MUTEX_FAST NULL
#  define CHEROKEE_MUTEX_ERRORCHECK NULL
# endif

extern pthread_key_t thread_error_writer_ptr;
extern pthread_key_t thread_connection_ptr;

/* Global if */
#endif

ret_t cherokee_threading_init (void);
ret_t cherokee_threading_free (void);

#endif /* CHEROKEE_BOGOTIME_H */
