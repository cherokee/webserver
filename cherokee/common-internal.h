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

#ifndef CHEROKEE_COMMON_INTERNAL_H
#define CHEROKEE_COMMON_INTERNAL_H

#include <config.h>
#include <constants.h>

#ifdef _WIN32
# include "unix4win32.h"
# include "win32_misc.h"
#endif

#include "common.h"

#ifndef _WIN32
# if defined HAVE_ENDIAN_H
#  include <endian.h>
# elif defined HAVE_MACHINE_ENDIAN_H
#  include <machine/endian.h>
# elif defined HAVE_SYS_ENDIAN_H
#  include <sys/endian.h>
# elif defined HAVE_SYS_ISA_DEFS_H
#  include <sys/isa_defs.h>
# else
#  error "Can not include endian.h"
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_VARARGS
# include <sys/varargs.h>
#endif

#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#elif HAVE_STDINT_H
# include <stdint.h>
#else
# error "Can not include inttypes or stdint"
#endif


#ifdef HAVE_PTHREAD
# include <pthread.h>
#endif

#ifdef HAVE_INLINE
# define INLINE inline
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

#ifdef HAVE_PTHREAD
# define CHEROKEE_MUTEX_T(n)          pthread_mutex_t n
# define CHEROKEE_RWLOCK_T(n)         pthread_rwlock_t n
# define CHEROKEE_THREAD_JOIN(t)      pthread_join(t,NULL)
# define CHEROKEE_THREAD_SELF         pthread_self()

# define CHEROKEE_MUTEX_LOCK(m)       pthread_mutex_lock(m)
# define CHEROKEE_MUTEX_UNLOCK(m)     pthread_mutex_unlock(m)
# define CHEROKEE_MUTEX_INIT(m,n)     pthread_mutex_init(m,n)
# define CHEROKEE_MUTEX_DESTROY(m)    pthread_mutex_destroy(m)
# define CHEROKEE_MUTEX_TRY_LOCK(m)   pthread_mutex_trylock(m)

# define CHEROKEE_RWLOCK_INIT(m,n)    pthread_rwlock_init(m,n)
# define CHEROKEE_RWLOCK_READER(m)    pthread_rwlock_rdlock(m)
# define CHEROKEE_RWLOCK_WRITER(m)    pthread_rwlock_wrlock(m)
# define CHEROKEE_RWLOCK_TRYREADER(m) pthread_rwlock_tryrdlock(m)
# define CHEROKEE_RWLOCK_TRYWRITER(m) pthread_rwlock_trywrlock(m)
# define CHEROKEE_RWLOCK_UNLOCK(m)    pthread_rwlock_unlock(m)
# define CHEROKEE_RWLOCK_DESTROY(m)   pthread_rwlock_destroy(m)
#else
# define CHEROKEE_MUTEX_T(n)          
# define CHEROKEE_RWLOCK_T(n)         
# define CHEROKEE_THREAD_JOIN(t)
# define CHEROKEE_THREAD_SELF         0

# define CHEROKEE_MUTEX_LOCK(m)
# define CHEROKEE_MUTEX_UNLOCK(m)
# define CHEROKEE_MUTEX_INIT(m,n)  
# define CHEROKEE_MUTEX_DESTROY(m) 
# define CHEROKEE_MUTEX_TRY_LOCK(m)   0

# define CHEROKEE_RWLOCK_INIT(m,n)
# define CHEROKEE_RWLOCK_READER(m)
# define CHEROKEE_RWLOCK_WRITER(m)
# define CHEROKEE_RWLOCK_TRYREADER(m) 0
# define CHEROKEE_RWLOCK_TRYWRITER(m) 0
# define CHEROKEE_RWLOCK_UNLOCK(m)
# define CHEROKEE_RWLOCK_DESTROY(m)
#endif

#ifdef _WIN32
# define SOCK_ERRNO()      WSAGetLastError()
#else
# define SOCK_ERRNO()      errno
#endif


/* IMPORTANT:
 * Cross compilers should define BYTE_ORDER in CFLAGS 
 */
#ifndef BYTE_ORDER

/* Definitions for byte order, according to byte significance from low
 * address to high.
 */
# ifndef  LITTLE_ENDIAN
#  define LITTLE_ENDIAN  1234    /* LSB first: i386, vax */
# endif
# ifndef BIG_ENDIAN
#  define BIG_ENDIAN     4321    /* MSB first: 68000, ibm, net */
# endif 
# ifndef PDP_ENDIAN
#  define PDP_ENDIAN     3412    /* LSB first in word, MSW first in long */
# endif

/* It assumes autoconf's AC_C_BIGENDIAN has been ran. 
 * If it hasn't, we assume the order is LITTLE ENDIAN.
 */
# ifdef WORDS_BIGENDIAN
#   define BYTE_ORDER  BIG_ENDIAN
# else
#   define BYTE_ORDER  LITTLE_ENDIAN
# endif

#endif

/* String missing prototypes: 
 * Implemented in util.c, can't move prototype there though
 */
#ifndef HAVE_STRSEP
char *strsep(char **str, const char *delims);
#endif

#ifndef HAVE_STRCASESTR
char *strcasestr(char *s, char *find);
#endif

/* Int limit
 */
#ifndef INT_MAX
# if (SIZEOF_INT == 4)
#  define INT_MAX 0x7fffffffL          /* 2^32 - 1 */
# elif (SIZEOF_INT == 8)
#  define INT_MAX 0x7fffffffffffffffL  /* 2^64 - 1 */
# else
#  error "Can't define INT_MAX"
# endif
#endif

#endif /* CHEROKEE_COMMON_INTERNAL_H */
