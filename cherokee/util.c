/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
#include "util.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else 
# include <time.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>     /* defines FIONBIO and FIONREAD */
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#if defined (HAVE_SYS_RESOURCE_H)
# include <sys/resource.h>
#elif defined (HAVE_RESOURCE_H)
# include <resource.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>         /* defines gethostbyname()  */
#endif

#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif

#if defined(HAVE_GNUTLS)
# include <gnutls/gnutls.h>
#elif defined(HAVE_OPENSSL)
# include <openssl/lhash.h>
# include <openssl/ssl.h>
# include <openssl/err.h>
# include <openssl/rand.h>
#endif


const char *cherokee_version    = PACKAGE_VERSION;
const char *cherokee_months[]   = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
				   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *cherokee_weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};



int
cherokee_hexit (char c)
{
	if ( c >= '0' && c <= '9' )
		return c - '0';
	if ( c >= 'a' && c <= 'f' )
		return c - 'a' + 10;
	if ( c >= 'A' && c <= 'F' )
		return c - 'A' + 10;

	/* shouldn't happen, we're guarded by isxdigit() */
	return 0;           
}


/* This function is licenced under:
 * The Apache Software License, Version 1.1
 *
 * Original name: apr_strfsize()
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  
 * All rights reserved.
 */
char *
cherokee_strfsize (unsigned long long size, char *buf)
{
	const char ord[] = "KMGTPE";
	const char *o = ord;
	int remain;

	if (size < 0) {
		return strcpy(buf, "  - ");
	}
	if (size < 973) {
		sprintf(buf, "%3d ", (int) size);
		return buf;
	}
	do {
		remain = (int)(size & 1023);
		size >>= 10;
		if (size >= 973) {
			++o;
			continue;
		}
		if (size < 9 || (size == 9 && remain < 973)) {
			if ((remain = ((remain * 5) + 256) / 512) >= 10)
				++size, remain = 0;
			sprintf(buf, "%d.%d%c", (int) size, remain, *o);
			return buf;
		}
		if (remain >= 512)
			++size;
		sprintf(buf, "%3d%c", (int) size, *o);
		return buf;
	} while (1);
}



char *
cherokee_min_str (char *s1, char *s2)
{
	if ((s1 == NULL) && 
	    (s2 == NULL)) return NULL;

	if ((s1 != NULL) && 
	    (s2 == NULL)) return s1;

	if ((s2 != NULL) && 
	    (s1 == NULL)) return s2;
	
	return (s1<s2) ? s1 : s2;
}


ret_t 
cherokee_sys_fdlimit_get (cuint_t *limit)
{
#ifdef HAVE_GETRLIMIT
        struct rlimit rlp;

        rlp.rlim_cur = rlp.rlim_max = RLIM_INFINITY;
        if (getrlimit(RLIMIT_NOFILE, &rlp))
            return ret_error;

        *limit = rlp.rlim_cur;
	return ret_ok;
#else
#ifdef HAVE_GETDTABLESIZE
	int nfiles;

	nfiles = getdtablesize();
        if (nfiles <= 0) return ret_error;

	*limit = nfiles;
	return ret_ok;
#else
#ifdef OPEN_MAX
        *limit = OPEN_MAX;         /* need to include limits.h somehow */
	return ret_ok;
#else
        *limit = FD_SETSIZE;  
	return ret_ok;
#endif
#endif
#endif
}


ret_t
cherokee_sys_fdlimit_set (cuint_t limit)
{
#ifndef _WIN32
	int           re;
	struct rlimit rl;

	rl.rlim_cur = limit;
	rl.rlim_max = limit;

	re = setrlimit (RLIMIT_NOFILE, &rl);
	if (re != 0) {
		return ret_error;
	}
#endif

	return ret_ok;
}


#ifndef HAVE_STRSEP
char *
strsep (char** str, const char* delims)
{
	char* token;

	if (*str == NULL) {
		/* No more tokens 
		 */
		return NULL;
	}
	
	token = *str;
	while (**str!='\0') {
		if (strchr(delims,**str)!=NULL) {
			**str='\0';
			(*str)++;
			return token;
		}
		(*str)++;
	}
	
	/* There is no other token 	
	 */
	*str=NULL;
	return token;
}
#endif


#ifndef HAVE_STRCASESTR
char *
strcasestr (register char *s, register char *find)
{
	register char c, sc;
	register size_t len;

	if ((c = *find++) != 0) {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return NULL;
			} while (sc != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *) s);
}
#endif


/* strlcat(): 
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */

#ifndef HAVE_STRLCAT
size_t
strlcat (char *dst, const char *src, size_t siz)
{
	   register char *d = dst;
        register const char *s = src;
        register size_t n = siz;
        size_t dlen;

        /* Find the end of dst and adjust bytes left but don't go past end */
        while (n-- != 0 && *d != '\0')
                d++;
        dlen = d - dst;
        n = siz - dlen;

        if (n == 0)
                return(dlen + strlen(s));
        while (*s != '\0') {
                if (n != 1) {
                        *d++ = *s;
                        n--;
                }
                s++;
        }
        *d = '\0';

        return(dlen + (s - src));       /* count does not include NUL */
}
#endif




#if defined(HAVE_PTHREAD) && !defined(HAVE_GMTIME_R)
static pthread_mutex_t gmtime_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

struct tm *
cherokee_gmtime (const time_t *timep, struct tm *result)
{
#ifndef HAVE_PTHREAD
	struct tm *tmp;

	tmp = gmtime (timep);
	memcpy (result, tmp, sizeof(struct tm));
	return result;	
#else 
# ifdef HAVE_GMTIME_R
	return gmtime_r (timep, result);
# else
	struct tm *tmp;

	CHEROKEE_MUTEX_LOCK (&gmtime_mutex);
	tmp = gmtime (timep);
	memcpy (result, tmp, sizeof(struct tm));
	CHEROKEE_MUTEX_UNLOCK (&gmtime_mutex);

	return result;
# endif
#endif
}


#if defined(HAVE_PTHREAD) && !defined(HAVE_LOCALTIME_R)
static pthread_mutex_t localtime_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

struct tm *
cherokee_localtime (const time_t *timep, struct tm *result)
{
#ifndef HAVE_PTHREAD
	struct tm *tmp;

	tmp = localtime (timep);
	memcpy (result, tmp, sizeof(struct tm));
	return result;	
#else 
# ifdef HAVE_LOCALTIME_R
	return localtime_r (timep, result);
# else
	struct tm *tmp;

	CHEROKEE_MUTEX_LOCK (&localtime_mutex);
	tmp = localtime (timep);
	memcpy (result, tmp, sizeof(struct tm));
	CHEROKEE_MUTEX_UNLOCK (&localtime_mutex);

	return result;
# endif
#endif
}




#if defined(HAVE_PTHREAD) && !defined(HAVE_READDIR_R)
static pthread_mutex_t readdir_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* The readdir subroutine is reentrant when an application program
 * uses different DirectoryPointer parameter values (returned from the
 * opendir subroutine). Use the readdir_r subroutine when multiple
 * threads use the same directory pointer.
 */

int 
cherokee_readdir (DIR *dirstream, struct dirent *entry, struct dirent **result)
{
#ifdef HAVE_POSIX_READDIR_R
	return readdir_r (dirstream, entry, result);

#else
# ifdef HAVE_OLD_READDIR_R
        int ret = 0;
        
        /* We cannot rely on the return value of readdir_r
	 * as it differs between various platforms
         * (HPUX returns 0 on success whereas Solaris returns non-zero)
         */
        entry->d_name[0] = '\0';
        readdir_r(dirp, entry);
        
        if (entry->d_name[0] == '\0') {
                *result = NULL;
                ret = errno;
        } else {
                *result = entry;
        }

        return ret;

# else
        struct dirent *ptr;
        int            ret = 0;
	
	CHEROKEE_MUTEX_LOCK (&readdir_mutex);
        
        errno = 0;        
        ptr = readdir(dirstream);
        
        if (!ptr && errno != 0)
                ret = errno;

        if (ptr)
                memcpy(entry, ptr, sizeof(*ptr));

        *result = ptr;
	
	CHEROKEE_MUTEX_UNLOCK (&readdir_mutex);
        return ret;
# endif
#endif
}


ret_t 
cherokee_split_pathinfo (cherokee_buffer_t  *path, 
			 int                 init_pos,
			 int                 allow_dirs,
			 char              **pathinfo,
			 int                *pathinfo_len)
{
	char        *cur;
	struct stat  st;
	char        *last_dir = NULL;
	
	for (cur = path->buf + init_pos; *cur; ++cur) {
		if (*cur != '/') continue;		
		*cur = '\0';

		/* Handle not found case
		 */
		if (stat (path->buf, &st) == -1) {
			*cur = '/';
			
			if ((allow_dirs) && (last_dir != NULL)) {
				*pathinfo = last_dir;
				*pathinfo_len = (path->buf + path->len) - last_dir;
				return ret_ok;
			}
			return ret_not_found;
		}

		/* Handle directory case
		 */
		if (S_ISDIR(st.st_mode)) {
			*cur = '/';
			last_dir = cur;
			continue;
		}
		
		/* Build the PathInfo string 
		 */
		*cur = '/';
		*pathinfo = cur;
		*pathinfo_len = (path->buf + path->len) - cur;
		return ret_ok;
	}

	*pathinfo_len = 0;
	return ret_ok;
}


ret_t 
cherokee_split_arguments (cherokee_buffer_t *request,
			  int                init_pos,
			  char             **arguments,
			  int               *arguments_len)
{
	char *tmp;
	char *p   = request->buf + init_pos;
	char *end = request->buf + request->len;

	tmp = strchr (p, '?');
	if (tmp == NULL) {
		*arguments     = NULL;
		*arguments_len = 0;
		return ret_ok;
	}

	*arguments = tmp+1;
	*arguments_len = end - *arguments;

	return ret_ok;
}


int
cherokee_estimate_va_length (char *fmt, va_list ap)
{
	char      *p;
	int        ch;
	cullong_t  ul;
	int        base, lflag, llflag, width;
	char       padc;
	unsigned   len = 0;

	for (;;) {
		padc = ' ';
		width = 0;
		while ((ch = *(char *)fmt++) != '%') {
			if (ch == '\0')
				return len+1;
			len++;
		}
		lflag = llflag = 0;

reswitch:	
		switch (ch = *(char *)fmt++) {
		case '0':
			padc = '0';
			goto reswitch;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for (width = 0;; ++fmt) {
				width = width * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9')
					break;
			}
			len += width;
			goto reswitch;
		case 'l':
			if (lflag == 0) 
				lflag = 1;
			else
				llflag = 1;
			goto reswitch;
		case 'c':
			va_arg(ap, int);
			len++;
			break;
		case 's':
			p = va_arg(ap, char *);
			len += strlen (p? p: "(null)");
			break;
		case 'd':
			ul = lflag ? va_arg(ap, culong_t) : va_arg(ap, int);
			if (ul < 0) {
				ul = -ul;
				len++;
			}
			base = 10;
			goto number;
		case 'o':
			ul = lflag ? va_arg(ap, culong_t) : va_arg(ap, int);
			base = 8;
			goto number;
		case 'u':
			if (llflag) {
				ul = va_arg(ap, cullong_t);
			} else {
				ul = lflag ? va_arg(ap, long) : va_arg(ap, int);
			}
			base = 10;
			goto number;
		case 'f':
			ul = va_arg(ap, double); // FIXME: Add float numbers support
			len += 30; 
			base = 10;
			goto number;			
		case 'p':
			len += 2; /* Pointer: "0x" + hex value */
		case 'x':
			ul = lflag ? va_arg(ap, culong_t) : va_arg(ap, int);
			base = 16;
number:
			do {
				ul /= base;
				len++;
			} while (ul > 0);
			len++;
			break;
			;
		case '%':
			len++;
		default:
			len+=2;
		}
	}

	return -1;
}



/* gethostbyname_r () emulation
 */
#if defined(HAVE_PTHREAD) && !defined(HAVE_GETHOSTBYNAME_R)
static pthread_mutex_t __global_gethostbyname_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

ret_t
cherokee_gethostbyname (const char *hostname, struct in_addr *addr)
{
#if !defined(HAVE_PTHREAD) || (defined(HAVE_PTHREAD) && !defined(HAVE_GETHOSTBYNAME_R))

        struct hostent *host;

        CHEROKEE_MUTEX_LOCK (&__global_gethostbyname_mutex);
        /* Resolv the host name
         */
        host = gethostbyname (hostname);
        if (host == NULL) {
                CHEROKEE_MUTEX_UNLOCK (&__global_gethostbyname_mutex);
                return ret_error;
        }

        /* Copy the address
         */
        memcpy (addr, host->h_addr, host->h_length);
        CHEROKEE_MUTEX_UNLOCK (&__global_gethostbyname_mutex);
        return ret_ok;

#elif defined(HAVE_PTHREAD) && defined(HAVE_GETHOSTBYNAME_R)

/* Maximum size that should use gethostbyname_r() function.
 * It will return ERANGE, if more space is needed.
 */
# define GETHOSTBYNAME_R_BUF_LEN 512

        int             r;
        int             h_errnop;
        struct hostent  hs;
        struct hostent *hp;
        char   tmp[GETHOSTBYNAME_R_BUF_LEN];
        

# ifdef SOLARIS
        /* Solaris 10:
         * struct hostent *gethostbyname_r
         *        (const char *, struct hostent *, char *, int, int *h_errnop);
         */
        hp = gethostbyname_r (hostname, &hs, tmp, 
			      GETHOSTBYNAME_R_BUF_LEN - 1, &h_errnop);
        if (hp == NULL) {
                return ret_error;
        }       
# else
        /* Linux glibc2:
         *  int gethostbyname_r (const char *name,
         *         struct hostent *ret, char *buf, size_t buflen,
         *         struct hostent **result, int *h_errnop);
         */
        r = gethostbyname_r (hostname, 
                             &hs, tmp, GETHOSTBYNAME_R_BUF_LEN - 1, 
                             &hp, &h_errnop);
        if (r != 0) {
                return ret_error;
        }
# endif  

        /* Copy the address
         */
        memcpy (addr, hp->h_addr, hp->h_length);

        return ret_ok;
#else

        SHOULDNT_HAPPEN;
        return ret_error;
#endif
}


#ifdef HAVE_GNUTLS
# ifdef GCRY_THREAD_OPTION_PTHREAD_IMPL
GCRY_THREAD_OPTION_PTHREAD_IMPL;
# endif
#endif

ret_t
cherokee_tls_init (void)
{	
#ifdef HAVE_GNUTLS
	int rc;

# ifdef GCRY_THREAD_OPTION_PTHREAD_IMPL
	/* Although the GnuTLS library is thread safe by design, some
	 * parts of the crypto backend, such as the random generator,
	 * are not; hence, it needs to initialize some semaphores.
	 */
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread); 
# endif

	/* Gnutls library-width initialization
	 */
	rc = gnutls_global_init();
	if (rc < 0) {
		PRINT_ERROR_S ("Global GNUTLS state initialisation failed.\n");
		return ret_error;
	}
#endif

#ifdef HAVE_OPENSSL
	SSL_load_error_strings();
	SSL_library_init();
	
	SSLeay_add_all_algorithms ();
	SSLeay_add_ssl_algorithms ();
#endif

	return ret_ok;
}


ret_t 
cherokee_fd_set_nonblocking (int fd)
{
	int tmp = 1;

#ifdef _WIN32
	tmp = ioctlsocket (fd, FIONBIO, (u_long)&tmp);
#else	
	tmp = ioctl (fd, FIONBIO, &tmp);
#endif
	if (tmp < 0) {
		PRINT_ERROR ("ERROR: Setting 'FIONBIO' in socked fd=%d\n", fd);
		return ret_error;
	}

	return ret_ok;
}


/* Return 1 if big-endian (Motorola and Sparc), not little-endian
 * (Intel and Vax).  We do this work at run-time, rather than at
 * configuration time so cross-compilation and general embedded system
 * support is simpler.
 */

int
cherokee_isbigendian (void)
{
	/* From Harbison & Steele.  
	 */
	union {                                 
                long l;
                char c[sizeof(long)];
        } u;

        u.l = 1;
        return (u.c[sizeof(long) - 1] == 1);
}



ret_t
cherokee_syslog (int priority, cherokee_buffer_t *buf)
{
	char *p;
	char *nl, *end;

	if (cherokee_buffer_is_empty(buf))
		return ret_ok;

	p   = buf->buf;
	end = buf->buf + buf->len;

	do {
		nl = strchr (p, '\n');
		if (nl != NULL) 
			*nl = '\0';

		syslog (priority, "%s", p);

		if (nl != NULL) 
			*nl = '\n';

		p = nl + 1;
	} while (p < end);

	return ret_ok;
}



#ifdef TRACE_ENABLED

void
cherokee_trace (const char *entry, const char *file, int line, const char *func, const char *fmt, ...)
{
	static char        *env         = NULL;
	static cuint_t      env_set     = 0;
	static cuint_t      use_syslog  = 0;
	cherokee_boolean_t  do_log      = false;
	cherokee_buffer_t   entries     = CHEROKEE_BUF_INIT;
	cherokee_buffer_t   output      = CHEROKEE_BUF_INIT;
	char               *lentry;
	char               *lentry_end;
	va_list             args;
	char               *p;
	
	/* Read the environment variable in the first call
	 */
	if (env_set == 0) {
		env     = getenv(TRACE_ENV);
		env_set = 1;

		if (env != NULL) 
			use_syslog = (strstr (env, "syslog") != NULL);
	}

	/* Return quickly if there isn't anything to do
	 */
	if (env == NULL) {
		return;
	}
       
	cherokee_buffer_add (&entries, (char *)entry, strlen(entry));

	for (lentry = entries.buf;;) {
		lentry_end = strchr (lentry, ',');
		if (lentry_end) *lentry_end = '\0';

		/* Check for 'all'
		 */
		p = strstr (env, "all");
		if (p) do_log = true;

		/* Check the type
		 */
		p = strstr (env, lentry);
		if (p) {
			char *tmp = p + strlen(lentry);
			if ((*tmp == '\0') || (*tmp == ',') || (*tmp == ' '))
				do_log = true;
		}

		if (lentry_end == NULL)
			break;

		lentry = lentry_end + 1;
	}

	if (! do_log) goto out;

	/* Print the trace
	 */
	cherokee_buffer_add_va (&output, "%18s:%04d (%30s): ", file, line, func);

	va_start (args, fmt);
	cherokee_buffer_add_va_list (&output, (char *)fmt, args);
	va_end (args);

	if (use_syslog) 
		syslog (LOG_DEBUG, "%s", output.buf);
	else
		printf ("%s", output.buf);

out:
	cherokee_buffer_mrproper (&output);
	cherokee_buffer_mrproper (&entries);
}

#endif



ret_t 
cherokee_short_path (cherokee_buffer_t *path)
{
	char *p   = path->buf;
	char *end = path->buf + path->len;

	while (p < end) {
		char    *dots_end;
		char    *prev_slash;
		cuint_t  len;
		
		if (p[0] != '.') {
			p++;
			continue;
		}

		if ((p[1] == '/') && (p > path->buf) && (*(p-1) == '/')) {
			cherokee_buffer_remove_chunk (path, p - path->buf, 2);
			p -= 1;
			continue;
		}

		if (end < p+2) {
			return ret_ok;
		}

		if (p[1] != '.') {
			p+=2;
			continue;
		}

		dots_end = p + 2;
		while ((dots_end < end) && (*dots_end == '.')) {
			dots_end++;
		}
		
		if (dots_end >= end)
			return ret_ok;

		prev_slash = p-1;

		if (prev_slash < path->buf)
			return ret_ok;
		
		if (*prev_slash != '/') {
			p = dots_end;
			continue;
		}

		if (prev_slash > path->buf)
			prev_slash--;
		
		while ((prev_slash > path->buf) && (*prev_slash != '/')) {
			prev_slash--;
		}
		
		len = dots_end - prev_slash;

		cherokee_buffer_remove_chunk (path, (prev_slash - path->buf), len);

		end = path->buf + path->len;
		p -= (len - (dots_end - p));
	}

	return ret_ok;
}


long * 
cherokee_get_timezone_ref (void)
{
#ifdef HAVE_INT_TIMEZONE
	return &timezone;
#else
	static long _faked_timezone = 0;
	return &_faked_timezone;
#endif
}


