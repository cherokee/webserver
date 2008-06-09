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

/* Some fragments of code from win32equ.[h,c]
 * Copyright (C)1999 by James Ewing <jim@ewingdata.com>
 */

#ifndef CHEROKEE_UNIX_4_WIN32
#define CHEROKEE_UNIX_4_WIN32

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ws2tcpip.h>
#include <process.h>
#include <io.h>
#include <direct.h>

/* The file descriptor limit has to be raised before the winsock2.h
 * header is included, otherwise the limit will be set to 64.
 */
#ifdef FD_SETSIZE
# undef FD_SETSIZE
#endif
#define FD_SETSIZE 1024

#include <winsock2.h>

#define _POSIX_
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

/* Don't need unix4win32_errno.h anymore. Only these errors
 * are needed.
 */
#define EWOULDBLOCK   WSAEWOULDBLOCK
#define EAFNOSUPPORT  WSAEAFNOSUPPORT
#define ETIMEDOUT     WSAETIMEDOUT
#define ECONNRESET    WSAECONNRESET
#define ECONNREFUSED  WSAECONNREFUSED
#define EHOSTUNREACH  WSAEHOSTUNREACH
#define ENOTSOCK      WSAENOTSOCK

/* syslog.h
 */
#define LOG_NOTICE      EVENTLOG_INFORMATION_TYPE
#define LOG_NDELAY      EVENTLOG_WARNING_TYPE
#define LOG_PID         EVENTLOG_WARNING_TYPE
#define LOG_EMERG       EVENTLOG_ERROR_TYPE
#define LOG_ALERT       EVENTLOG_WARNING_TYPE
#define LOG_CRIT        EVENTLOG_ERROR_TYPE
#define LOG_ERR         EVENTLOG_ERROR_TYPE
#define LOG_WARNING     EVENTLOG_WARNING_TYPE
#define LOG_INFO        EVENTLOG_INFORMATION_TYPE
#define LOG_DEBUG       EVENTLOG_INFORMATION_TYPE
#define LOG_KERN        EVENTLOG_WARNING_TYPE
#define LOG_USER        EVENTLOG_INFORMATION_TYPE
#define LOG_MAIL        EVENTLOG_INFORMATION_TYPE
#define LOG_DAEMON      EVENTLOG_INFORMATION_TYPE
#define LOG_AUTH        EVENTLOG_INFORMATION_TYPE
#define LOG_LPR         EVENTLOG_INFORMATION_TYPE
#define LOG_NEWS        EVENTLOG_INFORMATION_TYPE
#define LOG_UUCP        EVENTLOG_INFORMATION_TYPE
#define LOG_CRON        EVENTLOG_INFORMATION_TYPE
#define LOG_LOCAL0      EVENTLOG_INFORMATION_TYPE
#define LOG_LOCAL1      EVENTLOG_INFORMATION_TYPE
#define LOG_LOCAL2      EVENTLOG_INFORMATION_TYPE
#define LOG_LOCAL3      EVENTLOG_INFORMATION_TYPE
#define LOG_LOCAL4      EVENTLOG_INFORMATION_TYPE
#define LOG_CONS        0x1000

void openlog  (const char *ident, int logopt, int facility);
void syslog   (int priority, const char *message, ...);
void closelog (void);


/* Supplement to <sys/types.h>.
 */
#define uid_t   int
#define gid_t   int
#define pid_t   unsigned long
#define ssize_t int
#define mode_t  int
#define key_t   long
#define ushort  unsigned short

struct passwd {
	char *pw_name;   /* login user id  */
	char *pw_passwd; /* login password */
	char *pw_dir;    /* home directory */
	char *pw_shell;  /* login shell    */
	char *pw_gecos;
	int   pw_gid;
	int   pw_uid;
};

struct group {
	char *gr_name;   /* login user id  */
	char *gr_passwd; /* login password */
	int  gr_gid;
};

struct passwd *getpwuid (int uid);
struct passwd *getpwnam (const char *name);


/* Structure for scatter/gather I/O.
 */
struct iovec {
	void *iov_base;     /* Pointer to data.  */
	size_t iov_len;     /* Length of data.  */
};


typedef unsigned long in_addr_t;
int  getdtablesize (void);
int  setenv (const char *name, const char *value, int overwrite);

/* POSIX stuff we don't have
 */
#define setuid(x)   (0)
#define setsid(x)   (0)
#define setgid(x)   (0)
#define getuid(x)   (0)
#define getgid(x)   (0)
#define getgrgid(x) (0)
#define chroot(x)   (-1)
#define initgroups(usr,grp)  (0)

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

const char *inet_ntop (int af, const void *addr, char *buf, size_t size);
int         inet_pton (int af, const char *src, void *dst);
int         inet_aton (const char *cp, struct in_addr *addr);

/* <dlfcn.h> emulation
 */
#define RTLD_NOW  0
#define RTLD_LAZY 1

#define dlopen(dll,flg)  win_dlopen (dll, flg)
#define dlsym(hnd,func)  win_dlsym (hnd, func)
#define dlclose(hnd)     win_dlclose (hnd)
#define dlerror()        win_dlerror()

void       *win_dlopen  (const char *dll_name, int flags);
void       *win_dlsym   (const void *dll_handle, const char *func_name);
int         win_dlclose (const void *dll_handle);
const char *win_dlerror (void);

/* Unix mmap() emulation
 */
#define PROT_READ    0x1            /* page can be read */
#define PROT_WRITE   0x2            /* page can be written */
#define PROT_EXEC    0x4            /* page can be executed (not supported) */
#define PROT_NONE    0x0            /* page can not be accessed (not supported) */
#define MAP_SHARED   0x01           /* share changes (ot supported) */
#define MAP_PRIVATE  0x02           /* make mapping private (not supportd) */
#define MAP_FAILED   NULL

#define mmap(xx1,size,prot,xx2,fd,xx3)  win_mmap (fd,size,prot)
#define munmap(handle,size)             win_munmap ((const void*)(handle), size)

void *win_mmap (int fd, size_t size, int prot);
int   win_munmap (const void *handle, size_t size);

#endif /* CHEROKEE_UNIX_4_WIN32 */
