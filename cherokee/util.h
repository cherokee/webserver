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

#ifndef CHEROKEE_UTIL_H
#define CHEROKEE_UTIL_H

#include <cherokee/common.h>
#include <cherokee/avl.h>
#include <cherokee/trace.h>
#include <cherokee/plugin_loader.h>

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif 

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#include <time.h>
#include <dirent.h>
#include <errno.h>

#include <cherokee/buffer.h>

CHEROKEE_BEGIN_DECLS

/* Error buffer size for cherokee_strerror_r().
 */
#define ERROR_MIN_BUFSIZE	64	/* min. buffer size */
#define ERROR_MAX_BUFSIZE	512	/* max. buffer size */

#ifdef _WIN32
# define cherokee_stat(path,buf)   cherokee_win32_stat(path,buf)
# define cherokee_lstat(path,buf)  cherokee_win32_stat(path,buf)
# define cherokee_error            GetLastError()
# define cherokee_mkdir(path,perm) mkdir(path)
#else
# define cherokee_stat(path,buf)   stat(path,buf)
# define cherokee_lstat(path,buf)  lstat(path,buf)
# define cherokee_error            errno
# define cherokee_mkdir(path,perm) mkdir(path,perm)
#endif

/* Some global information
 */

/* System
 */
ret_t cherokee_tls_init (void);

/* String management functions
 */
char   *cherokee_strerror_r         (int err, char *buf, size_t bufsize);
int     cherokee_isbigendian        (void);
char   *cherokee_min_str            (char *s1, char *s2);
char   *cherokee_max_str            (char *s1, char *s2);
size_t  cherokee_strlcat            (char *dst, const char *src, size_t siz);
int     cherokee_estimate_va_length (char *format, va_list ap);
long    cherokee_eval_formated_time (cherokee_buffer_t *buf);  
ret_t   cherokee_fix_dirpath        (cherokee_buffer_t *buf);

/* Time management functions
 */
struct tm *cherokee_gmtime           (const time_t *timep, struct tm *result);
struct tm *cherokee_localtime        (const time_t *timep, struct tm *result);
long      *cherokee_get_timezone_ref (void);

/* Thread safe functions
 */
int   cherokee_readdir       (DIR *dirstream, struct dirent *entry, struct dirent **result);
ret_t cherokee_gethostbyname (const char *hostname, void *addr);
ret_t cherokee_syslog        (int priority, cherokee_buffer_t *buf);
ret_t cherokee_getpwnam      (const char *name, struct passwd *pwbuf, char *buf, size_t buflen);
ret_t cherokee_getgrnam      (const char *name, struct group *pwbuf, char *buf, size_t buflen);
ret_t cherokee_mkstemp       (cherokee_buffer_t *buffer, int *fd);
ret_t cherokee_mkdir_p       (cherokee_buffer_t *path);

/* File descriptors
 */
ret_t cherokee_fd_set_nonblocking (int fd, cherokee_boolean_t enable);
ret_t cherokee_fd_set_nodelay     (int fd, cherokee_boolean_t enable);
ret_t cherokee_fd_set_closexec    (int fd);
ret_t cherokee_fd_close           (int fd);

/* Misc
 */
ret_t cherokee_sys_fdlimit_get (cuint_t *limit);
ret_t cherokee_sys_fdlimit_set (cuint_t  limit);
ret_t cherokee_get_shell       (const char **shell, const char **binary);
void  cherokee_print_wrapped   (cherokee_buffer_t *buffer);

/* IO vectors
 */
ret_t cherokee_iovec_skip_sent (struct iovec orig[], uint16_t  orig_len,
				struct iovec dest[], uint16_t *dest_len,
				size_t sent);
ret_t cherokee_iovec_was_sent  (struct iovec orig[], uint16_t orig_len, size_t sent);

/* Debug
 */
void  cherokee_trace           (const char *entry, const char *file, int line, const char *func, const char *fmt, ...);
void  cherokee_print_errno     (int error, char *format, ...);

/* Path management
 */
ret_t cherokee_path_short         (cherokee_buffer_t *path);
ret_t cherokee_path_arg_eval      (cherokee_buffer_t *path);

/* Path walking
 */
ret_t cherokee_split_pathinfo     (cherokee_buffer_t  *path, 
				   cuint_t             init_pos,
				   int                 allow_dirs,
				   char              **pathinfo,
				   int                *pathinfo_len);

ret_t cherokee_split_arguments    (cherokee_buffer_t *request,
				   int                init_pos,
				   char             **arguments,
				   int               *arguments_len);

ret_t cherokee_parse_query_string (cherokee_buffer_t *qstring, 
				   cherokee_avl_t  *arguments);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_UTIL_H */
