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

#ifndef CHEROKEE_COMMON_INTERNAL_H
#define CHEROKEE_COMMON_INTERNAL_H

#include <config.h>
#include <constants.h>

#include "common.h"
#include "threading.h"
#include "error_log.h"

#if defined HAVE_ENDIAN_H
# include <endian.h>
#elif defined HAVE_MACHINE_ENDIAN_H
# include <machine/endian.h>
#elif defined HAVE_SYS_ENDIAN_H
# include <sys/endian.h>
#elif defined HAVE_SYS_MACHINE_H
# include <sys/machine.h>
#elif defined HAVE_SYS_ISA_DEFS_H
# include <sys/isa_defs.h>
#else
# error "Can not include endian.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_VARARGS
# include <sys/varargs.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
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

#ifdef HAVE_SCHED_H
# include <sched.h>
#endif

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#ifdef HAVE_INLINE
# define INLINE inline
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

#ifdef HAVE_PTHREAD
# define CHEROKEE_MUTEX_T(n)           pthread_mutex_t n
# define CHEROKEE_RWLOCK_T(n)          pthread_rwlock_t n
# define CHEROKEE_THREAD_JOIN(t)       pthread_join(t,NULL)
# define CHEROKEE_THREAD_KILL(t,s)     pthread_kill(t,s)
# define CHEROKEE_THREAD_SELF          pthread_self()

# define CHEROKEE_THREAD_PROP_GET(p)   pthread_getspecific(p)
# define CHEROKEE_THREAD_PROP_SET(p,v) pthread_setspecific(p,v)
# define CHEROKEE_THREAD_PROP_NEW(p,f) pthread_key_create2(p,f)

# define CHEROKEE_MUTEX_LOCK(m)        pthread_mutex_lock(m)
# define CHEROKEE_MUTEX_UNLOCK(m)      pthread_mutex_unlock(m)
# define CHEROKEE_MUTEX_INIT(m,n)      pthread_mutex_init(m,n)
# define CHEROKEE_MUTEX_DESTROY(m)     pthread_mutex_destroy(m)
# define CHEROKEE_MUTEX_TRY_LOCK(m)    pthread_mutex_trylock(m)

# define CHEROKEE_RWLOCK_INIT(m,n)                       \
	do {                                             \
		memset (m, 0, sizeof(pthread_rwlock_t)); \
		pthread_rwlock_init(m,n);                \
	} while(0)
# define CHEROKEE_RWLOCK_READER(m)     pthread_rwlock_rdlock(m)
# define CHEROKEE_RWLOCK_WRITER(m)     pthread_rwlock_wrlock(m)
# define CHEROKEE_RWLOCK_TRYREADER(m)  pthread_rwlock_tryrdlock(m)
# define CHEROKEE_RWLOCK_TRYWRITER(m)  pthread_rwlock_trywrlock(m)
# define CHEROKEE_RWLOCK_UNLOCK(m)     pthread_rwlock_unlock(m)
# define CHEROKEE_RWLOCK_DESTROY(m)    pthread_rwlock_destroy(m)
#else
# define CHEROKEE_MUTEX_T(n)
# define CHEROKEE_RWLOCK_T(n)
# define CHEROKEE_THREAD_JOIN(t)
# define CHEROKEE_THREAD_SELF          0
# define CHEROKEE_THREAD_KILL(t,s)     0

# define CHEROKEE_THREAD_PROP_GET(p)   NULL
# define CHEROKEE_THREAD_PROP_SET(p,v) NULL
# define CHEROKEE_THREAD_PROP_NEW(p,f) 0

# define CHEROKEE_MUTEX_LOCK(m)
# define CHEROKEE_MUTEX_UNLOCK(m)
# define CHEROKEE_MUTEX_INIT(m,n)
# define CHEROKEE_MUTEX_DESTROY(m)
# define CHEROKEE_MUTEX_TRY_LOCK(m)    0

# define CHEROKEE_RWLOCK_INIT(m,n)
# define CHEROKEE_RWLOCK_READER(m)
# define CHEROKEE_RWLOCK_WRITER(m)
# define CHEROKEE_RWLOCK_TRYREADER(m)  0
# define CHEROKEE_RWLOCK_TRYWRITER(m)  0
# define CHEROKEE_RWLOCK_UNLOCK(m)
# define CHEROKEE_RWLOCK_DESTROY(m)
#endif

#ifdef HAVE_SCHED_YIELD
# define CHEROKEE_THREAD_YIELD         sched_yield()
#else
# define CHEROKEE_THREAD_YIELD
#endif


# define SOCK_ERRNO()      errno


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

/* Long limit
 */
#ifndef LONG_MAX
# if (SIZEOF_LONG == 4)
#  define LONG_MAX 0x7fffffffL
# elif (SIZEOF_LONG == 8)
#  define LONG_MAX 0x7fffffffffffffffL
# else
#  error "Can't define LONG_MAX"
# endif
#endif

/* time_t limit
 */
#ifndef TIME_MAX
# if (SIZEOF_TIME_T == SIZEOF_INT)
#  define TIME_MAX ((time_t)INT_MAX)
# elif (SIZEOF_TIME_T == SIZEOF_LONG)
#  define TIME_MAX ((time_t)LONG_MAX)
# else
#  error "Can't define TIME_MAX"
# endif
#endif

/* Depend on fcntl.h
 */
#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0
#endif

#ifndef O_LARGEFILE
# define O_LARGEFILE 0
#endif

#ifndef S_ISLNK
# define S_ISLNK(i) (0)
#endif

/* SysV semaphores
 */
#ifndef SEM_R
# define SEM_R 0400
#endif

#ifndef SEM_A
# define SEM_A 0200
#endif

/* GCC specific
 */
#ifndef NORETURN
# ifdef __GNUC__
#  define NORETURN __attribute__((noreturn))
# else
#  define NORETURN
# endif
#endif


#endif /* CHEROKEE_COMMON_INTERNAL_H */
