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
#include "util.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else 
# include <time.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif

#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>     /* defines FIONBIO and FIONREAD */
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
# include <gcrypt.h>
# include <gnutls/gnutls.h>
#elif defined(HAVE_OPENSSL)
# include <openssl/lhash.h>
# include <openssl/ssl.h>
# include <openssl/err.h>
# include <openssl/rand.h>
#if HAVE_OPENSSL_ENGINE_H
# include <openssl/engine.h>
#endif
#endif

#define ENTRIES "util"

const char *cherokee_version    = PACKAGE_VERSION;

/* Given an error number (errno) it returns an error string.
 * Parameters "buf" and "bufsize" are passed by caller
 * in order to make the function "thread safe".
 * If the error number is unknown
 * then an "Unknown error nnn" string is returned.
 */
char *
cherokee_strerror_r (int err, char *buf, size_t bufsize)
{
#ifdef _WIN32
	return win_strerror (err, buf, bufsize);
#else
	char *p;
	if (buf == NULL)
		return NULL;

	if (bufsize < ERROR_MIN_BUFSIZE)
		return NULL;

	p = strerror(err);
	if (p == NULL) {
		buf[0] = '\0';
		snprintf (buf, bufsize, "Unknown error %d (errno)", err);
		buf[bufsize-1] = '\0';
		return buf;
	}

	return p;
#endif
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


char *
cherokee_max_str (char *s1, char *s2)
{
	if ((s1 == NULL) && 
	    (s2 == NULL)) return NULL;

	if ((s1 != NULL) && 
	    (s2 == NULL)) return s1;

	if ((s2 != NULL) && 
	    (s1 == NULL)) return s2;
	
	return (s1>s2) ? s1 : s2;
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


/* Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
cherokee_strlcat (char *dst, const char *src, size_t siz)
{
#ifdef HAVE_STRLCAT
	return strlcat (dst, src, siz);
#else
	/* The following few lines has been copy and pasted from Todd
	 * C. Miller <Todd.Miller@courtesan.com> code. BSD licensed.
	 */
	register char *d = dst;
        register const char *s = src;
        register size_t n = siz;
        size_t dlen;

        /* Find the end of dst and adjust bytes left but do not go
	 * past end.
	 */
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

	 /* Count does not include NUL 
	  */
        return(dlen + (s - src));      
#endif
}


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
#ifdef HAVE_GMTIME_R
	/* Use the thread safe version anyway (no data copy).
	 */
	return gmtime_r (timep, result);
#else
	struct tm *tmp;

	CHEROKEE_MUTEX_LOCK (&gmtime_mutex);
	if (likely ((tmp = gmtime (timep)) != NULL))
		memcpy (result, tmp, sizeof(struct tm));
	CHEROKEE_MUTEX_UNLOCK (&gmtime_mutex);

	return (tmp == NULL ? NULL : result);
#endif
}


#if defined(HAVE_PTHREAD) && !defined(HAVE_LOCALTIME_R)
static pthread_mutex_t localtime_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

struct tm *
cherokee_localtime (const time_t *timep, struct tm *result)
{
#ifdef HAVE_LOCALTIME_R
	/* Use the thread safe version anyway (no data copy).
	 */
	return localtime_r (timep, result);
#else
	struct tm *tmp;

	CHEROKEE_MUTEX_LOCK (&localtime_mutex);
	if (likely ((tmp = localtime (timep)) != NULL))
		memcpy (result, tmp, sizeof(struct tm));
	CHEROKEE_MUTEX_UNLOCK (&localtime_mutex);

	return (tmp == NULL ? NULL : result);
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
#ifndef HAVE_READDIR
# warning "readdir() unimplemented"
	return ENOSYS;
#else
# ifdef HAVE_READDIR_R_2
        /* We cannot rely on the return value of readdir_r as it
	 * differs between various platforms (HPUX returns 0 on
	 * success whereas Solaris returns non-zero)
         */
        entry->d_name[0] = '\0';

        readdir_r (dirstream, entry);

	if (entry->d_name[0] != '\0') {
                *result = entry;
		return 0;
	}

	*result = NULL;
	return errno;

# elif defined(HAVE_READDIR_R_3)
	return readdir_r (dirstream, entry, result);
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
			 cuint_t             init_pos,
			 int                 allow_dirs,
			 char              **pathinfo,
			 int                *pathinfo_len)
{
	char        *cur;
	struct stat  st;
	char        *last_dir = NULL;
	
	if (init_pos > path->len)
		return ret_not_found;

	for (cur = path->buf + init_pos; *cur && (cur < path->buf + path->len); cur++) {
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
	char               *p;
	cuchar_t            ch;
	cllong_t            ll;
	cullong_t           ul;
	cherokee_boolean_t  lflag;
	cherokee_boolean_t  llflag;
	cuint_t             width;
	char                padc;
	cuint_t             len = 0;

#define LEN_NUM(var,base)    \
	do {                 \
		var /= base; \
		len++;       \
        } while (var > 0);   \
	len++

	for (;;) {
		width = 0;
		padc  = ' ';

		while ((ch = *fmt++) != '%') {
			if (ch == '\0')
				return len+1;
			len++;
		}
		lflag = llflag = false;

reswitch:	
		switch (ch = *fmt++) {
		case 's':
			p = va_arg(ap, char *);
			len += strlen (p ? p : "(null)");
			break;
		case 'd':
			ll = lflag ? va_arg(ap, clong_t) : va_arg(ap, int);
		        if (unlikely (ll < 0)) {
				ll = -ll;
				len++;
			} 
			LEN_NUM(ll,10);
			break;
		case 'l':
			if (lflag == false) 
				lflag = true;
			else
				llflag = true;
			goto reswitch;
		case 'u':
			if (llflag) {
				ul = va_arg(ap, cullong_t);
			} else {
				ul = lflag ? va_arg(ap, long) : va_arg(ap, int);
			}
			LEN_NUM(ul,10);
			break;
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
		case 'c':
			(void) va_arg(ap, int);
			len++;
			break;
		case 'o':
			ll = lflag ? va_arg(ap, clong_t) : va_arg(ap, int);
		        if (unlikely (ll < 0)) {
				ll = -ll;
				len++;
			} 
		        LEN_NUM(ll,8);
			break;
		case 'f':
			ul = va_arg(ap, double); /* FIXME: Add float numbers support */
			len += 30; 
			LEN_NUM(ul,10);
			break;
		case 'p':
			len += 2;                /* Pointer: "0x" + hex value */
			if (sizeof(void *) > sizeof(int))
				lflag = true;
		case 'x':
			ll = lflag ? va_arg(ap, clong_t) : va_arg(ap, int);
		        if (unlikely (ll < 0)) {
				ll = -ll;
				len++;
			} 
		        LEN_NUM(ll,16);
			break;
		case '%':
			len++;
		default:
			len+=2;
		}
	}

	return -1;
}


long
cherokee_eval_formated_time (cherokee_buffer_t *buf)
{
	char end;
	int  mul = 1;

	if (unlikely (cherokee_buffer_is_empty (buf)))
		return ret_ok;
	
	end = cherokee_buffer_end_char (buf);
	switch (end) {
	case 's':
		mul = 1;
		break;
	case 'm':
		mul = 60;
		break;
	case 'h':
		mul = 60 * 60;
		break;
	case 'd':
		mul = 60 * 60 * 24;
		break;
	case 'w':
		mul = 60 * 60 * 24 * 7;
		break;
	default:
		break;
	}

	return atol(buf->buf) * mul;
}



/* gethostbyname_r () emulation
 */
#if defined(HAVE_PTHREAD) && !defined(HAVE_GETHOSTBYNAME_R)
static pthread_mutex_t __global_gethostbyname_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

ret_t
cherokee_gethostbyname (const char *hostname, void *_addr)
{
	struct in_addr *addr = _addr;
	
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
	struct hostent *hp = NULL;
	char   tmp[GETHOSTBYNAME_R_BUF_LEN];
        

# if defined(SOLARIS) || defined(IRIX)
	/* Solaris 10:
	 * struct hostent *gethostbyname_r
	 *        (const char *, struct hostent *, char *, int, int *h_errnop);
	 */
	hp = gethostbyname_r (hostname, &hs, tmp, 
			GETHOSTBYNAME_R_BUF_LEN - 1, &h_errnop);

	if (hp == NULL)
		return ret_error;

# else
	/* Linux glibc2:
	 *  int gethostbyname_r (const char *name,
	 *         struct hostent *ret, char *buf, size_t buflen,
	 *         struct hostent **result, int *h_errnop);
	 */
	r = gethostbyname_r (hostname, 
			&hs, tmp, GETHOSTBYNAME_R_BUF_LEN - 1, 
			&hp, &h_errnop);
	if (r != 0)
		return ret_error;
# endif  
	/* Copy the address
	 */
	if (hp == NULL)
		return ret_not_found;

	memcpy (addr, hp->h_addr, hp->h_length);

	return ret_ok;
#else
	/* Bad case !
	 */
	SHOULDNT_HAPPEN;
	return ret_error;
#endif
}


#if defined(HAVE_GNUTLS) && defined (HAVE_PTHREAD)
# ifdef GCRY_THREAD_OPTION_PTHREAD_IMPL
GCRY_THREAD_OPTION_PTHREAD_IMPL;
# endif
#endif

ret_t
cherokee_tls_init (void)
{
#if HAVE_OPENSSL_ENGINE_H
	ENGINE *e;
#endif
#ifdef HAVE_GNUTLS
	int rc;

# ifdef HAVE_PTHREAD
#  ifdef GCRY_THREAD_OPTION_PTHREAD_IMPL
	/* Although the GnuTLS library is thread safe by design, some
	 * parts of the crypto backend, such as the random generator,
	 * are not; hence, it needs to initialize some semaphores.
	 */
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread); 
#  endif
# endif

	/* Try to speed up random number generation. On Linux, it
	 * takes ages for GNUTLS to get the random numbers it needs.
	 */
	gcry_control (GCRYCTL_ENABLE_QUICK_RANDOM, 0);
 
	/* Gnutls library-width initialization
	 */
	rc = gnutls_global_init();
	if (rc < 0) {
		PRINT_ERROR_S ("Global GNUTLS state initialisation failed.\n");
		return ret_error;
	}
#endif

#ifdef HAVE_OPENSSL
# if HAVE_OPENSSL_ENGINE_H
#  if OPENSSL_VERSION_NUMBER >= 0x00907000L
	ENGINE_load_builtin_engines();
#  endif
	e = ENGINE_by_id("pkcs11");
	if (e != NULL) {
		if(!ENGINE_init(e)) {
			ENGINE_free(e);
			PRINT_ERROR_S ("could not init pkcs11 engine");
			return ret_error;
		}
		
		if(!ENGINE_set_default(e, ENGINE_METHOD_ALL)) {
			ENGINE_free(e);
			PRINT_ERROR_S ("could not set all defaults");
			return ret_error;
		}
		
		ENGINE_finish(e);
		ENGINE_free(e);
	}
# endif

	SSL_load_error_strings();
	SSL_library_init();
	
	SSLeay_add_all_algorithms ();
	SSLeay_add_ssl_algorithms ();
#endif
	return ret_ok;
}


ret_t
cherokee_fd_set_nodelay (int fd, cherokee_boolean_t enable)
{
	int re;
	int flags;

	/* TCP_NODELAY: Disable the Nagle algorithm. This means that
         * segments are always sent as soon as possible, even if there
         * is only a small amount of data.  When not set, data is
         * buffered until there is a sufficient amount to send out,
         * thereby avoiding the frequent sending of small packets,
         * which results in poor utilization of the network.
	 */
#ifdef _WIN32
	re = ioctlsocket (fd, FIONBIO, (u_long) &enable);
#else
 	flags = fcntl (fd, F_GETFL, 0);
	if (unlikely (flags == -1)) {
		PRINT_ERRNO (errno, "ERROR: fcntl/F_GETFL fd %d: ${errno}\n", fd);
		return ret_error;
	}

	if (enable)
		BIT_SET (flags, O_NDELAY);
	else 
		BIT_UNSET (flags, O_NDELAY);
	
	re = fcntl (fd, F_SETFL, flags);
#endif	
	if (unlikely (re < 0)) {
		PRINT_ERRNO (errno, "ERROR: Setting O_NDELAY to fd %d: ${errno}\n", fd);
		return ret_error;
	}

	return ret_ok;
}

ret_t 
cherokee_fd_set_nonblocking (int fd, cherokee_boolean_t enable)
{
	int re;
	int flags = 0;

#ifdef _WIN32
	re = ioctlsocket (fd, FIONBIO, (u_long *) &enable);
#else	
	flags = fcntl (fd, F_GETFL, 0);
	if (flags < 0) {
		PRINT_ERRNO (errno, "ERROR: fcntl/F_GETFL fd %d: ${errno}\n", fd);
		return ret_error;
	}

	if (enable)
		BIT_SET (flags, O_NONBLOCK);
	else
		BIT_UNSET (flags, O_NONBLOCK);

	re = fcntl (fd, F_SETFL, flags);
#endif
	if (re < 0) {
		PRINT_ERRNO (errno, "ERROR: Setting O_NONBLOCK to fd %d: ${errno}\n", fd);
		return ret_error;
	}

	return ret_ok;
}


ret_t 
cherokee_fd_set_closexec (int fd)
{
#ifndef _WIN32
	int re;

	re = fcntl (fd, F_SETFD, FD_CLOEXEC);
	if (re < 0) {
		PRINT_ERRNO (errno, "ERROR: Setting FD_CLOEXEC to fd %d: ${errno}\n", fd);
		return ret_error;
	}
#endif

	return ret_ok;
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


ret_t 
cherokee_path_short (cherokee_buffer_t *path)
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


ret_t
cherokee_path_arg_eval (cherokee_buffer_t *path)
{
	ret_t  ret;
	char  *d;
	char   tmp[512];

	if (path->buf[0] != '/') {
		d = getcwd (tmp, sizeof(tmp));

		cherokee_buffer_prepend (path, "/", 1);		
		cherokee_buffer_prepend (path, d, strlen(d));		
	}

	ret = cherokee_path_short (path);
	if (ret != ret_ok)
		return ret_error;

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


ret_t 
cherokee_parse_query_string (cherokee_buffer_t *qstring, cherokee_avl_t *arguments)
{
 	char *string;
	char *token; 

	if (cherokee_buffer_is_empty (qstring))
		return ret_ok;

	string = qstring->buf;

	while ((token = (char *) strsep(&string, "&")) != NULL) {
		char *equ, *key, *val;

		if (*token == '\0')
			continue;

		if ((equ = strchr(token, '=')) != NULL) {
			*equ = '\0';

			key = token;
			val = equ+1;

			cherokee_avl_add_ptr (arguments, key, strdup(val));

			*equ = '=';
		} else {
			cherokee_avl_add_ptr (arguments, token, NULL);
		}

		/* UGLY hack, part 1:
		 * It restore the string modified by the strtok() function
		 */
		token[strlen(token)] = '&';
	}

	/* UGLY hack, part 2:
	 */
	qstring->buf[qstring->len] = '\0';
	return ret_ok;
}



#if defined(HAVE_PTHREAD) && !defined(HAVE_GETPWNAM_R)
static pthread_mutex_t __global_getpwnam_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

ret_t 
cherokee_getpwnam (const char *name, struct passwd *pwbuf, char *buf, size_t buflen)
{
#ifndef HAVE_GETPWNAM_R
	size_t         pw_name_len   = 0;
	size_t         pw_passwd_len = 0;
	size_t         pw_gecos_len  = 0;
	size_t         pw_dir_len    = 0;
	size_t         pw_shell_len  = 0;
	char          *ptr;
	struct passwd *tmp;

	CHEROKEE_MUTEX_LOCK (&__global_getpwnam_mutex);
	
	tmp = getpwnam (name);
	if (tmp == NULL) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getpwnam_mutex);
		return ret_error;
	}
	if (tmp->pw_name)   pw_name_len   = strlen(tmp->pw_name);
	if (tmp->pw_passwd) pw_passwd_len = strlen(tmp->pw_passwd);
	if (tmp->pw_gecos)  pw_gecos_len  = strlen(tmp->pw_gecos);
	if (tmp->pw_dir)    pw_dir_len    = strlen(tmp->pw_dir);
	if (tmp->pw_shell)  pw_shell_len  = strlen(tmp->pw_shell);

	if ((pw_name_len + pw_passwd_len + 
	     pw_gecos_len + pw_dir_len + pw_shell_len + 5) >= buflen) {
		/* Buffer overflow.
		 */
		CHEROKEE_MUTEX_UNLOCK (&__global_getpwnam_mutex);
		return ret_error;
	}
	memset (buf, 0, buflen);
	ptr = buf;

	pwbuf->pw_uid = tmp->pw_uid;
	pwbuf->pw_gid = tmp->pw_gid;

	if (tmp->pw_dir) {
		memcpy (ptr, tmp->pw_dir, pw_dir_len);
		pwbuf->pw_dir = ptr;
		ptr += pw_dir_len + 1;
	}

	if (tmp->pw_passwd) {
		memcpy (ptr, tmp->pw_passwd, pw_passwd_len);
		pwbuf->pw_passwd = ptr;
		ptr += pw_passwd_len + 1;
	}

	if (tmp->pw_name) {
		memcpy (ptr, tmp->pw_name, pw_name_len);
		pwbuf->pw_name = ptr;
		ptr += pw_name_len + 1;
	}

	if (tmp->pw_gecos) {
		memcpy (ptr, tmp->pw_gecos, pw_gecos_len);
		pwbuf->pw_gecos = ptr;
		ptr += pw_gecos_len + 1;
	}

	if (tmp->pw_shell) {
		memcpy (ptr, tmp->pw_shell, pw_shell_len);
		pwbuf->pw_shell = ptr;
		ptr += pw_shell_len + 1;
	}

	CHEROKEE_MUTEX_UNLOCK (&__global_getpwnam_mutex);
	return ret_ok;

#elif HAVE_GETPWNAM_R_5
	int            re;
	struct passwd *tmp = NULL;

	/* MacOS X:
	 * int getpwnam_r (const char     *login, 
	 *                 struct passwd  *pwd, 
	 *                 char           *buffer, 
	 *                 size_t          bufsize,
	 *                 struct passwd **result);
	 *
	 * Linux: 
	 * int getpwnam_r (const char     *name, 
	 * 	           struct passwd  *pwbuf, 
	 *                 char           *buf,
         *                 size_t          buflen,
	 *                 struct passwd **pwbufp);
	 */
	re = getpwnam_r (name, pwbuf, buf, buflen, &tmp);
	if ((re != 0) || (tmp == NULL)) 
		return ret_error;

	return ret_ok;

#elif HAVE_GETPWNAM_R_4 
	struct passwd * result;

#ifdef _POSIX_PTHREAD_SEMANTICS
	int re;
	re = getpwnam_r (name, pwbuf, buf, buflen, &result);
	if (re != 0) return ret_error;
#else
	result = getpwnam_r (name, pwbuf, buf, buflen);
	if (result == NULL) return ret_error;
#endif

	return ret_ok;
#endif

	return ret_no_sys;
}


#if defined(HAVE_PTHREAD) && !defined(HAVE_GETGRNAM_R)
static pthread_mutex_t __global_getgrnam_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

ret_t 
cherokee_getgrnam (const char *name, struct group *grbuf, char *buf, size_t buflen)
{
#ifndef HAVE_GETGRNAM_R
	size_t        gr_name_len   = 0;
	size_t        gr_passwd_len = 0;
	char         *ptr;
	struct group *tmp;

	CHEROKEE_MUTEX_LOCK (&__global_getgrnam_mutex);

	tmp = getgrnam (name);
	if (tmp == NULL) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getgrnam_mutex);
		return ret_error;
	}

	if (tmp->gr_name)
		gr_name_len   = strlen(tmp->gr_name);
	if (tmp->gr_passwd)
		gr_passwd_len = strlen(tmp->gr_passwd);

	if ((gr_name_len + gr_passwd_len + 2) >= buflen) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getgrnam_mutex);
		return ret_error;
	}
	memset (buf, 0, buflen);
	ptr = buf;

	grbuf->gr_gid = tmp->gr_gid;

	if (tmp->gr_name) {
		memcpy (ptr, tmp->gr_name, gr_name_len);
		grbuf->gr_name = ptr;
		ptr += gr_name_len + 1;
	}

	if (tmp->gr_passwd) {
		memcpy (ptr, tmp->gr_passwd, gr_passwd_len);
		grbuf->gr_passwd = ptr;
		ptr += gr_passwd_len + 1;
	}

	/* TODO: Duplicate char **tmp->gr_mem
	 */

	CHEROKEE_MUTEX_UNLOCK (&__global_getgrnam_mutex);

	return ret_ok;

#elif HAVE_GETGRNAM_R_5
	int           re;
	struct group *tmp;

	re = getgrnam_r (name, grbuf, buf, buflen, &tmp);
	if (re != 0)
		return ret_error;

	return ret_ok;

#elif HAVE_GETGRNAM_R_4
	struct group  *result;

#ifdef _POSIX_PTHREAD_SEMANTICS
	int re;
	re = getgrnam_r (name, grbuf, buf, buflen, &result);
	if (re != 0)
		return ret_error;
#else
	result = getgrnam_r (name, grbuf, buf, buflen);
	if (result == NULL)
		return ret_error;	
#endif

	return ret_ok;
#endif

	return ret_no_sys;
}


ret_t 
cherokee_fd_close (int fd)
{
	int re;
	
	if (unlikely (fd < 0)) {
		return ret_error;
	}
	
#ifdef _WIN32
	re = closesocket (fd);
#else  
	re = close (fd);
#endif

	TRACE (ENTRIES",close_fd", "fd=%d re=%d\n", fd, re);
	
	return (re == 0) ? ret_ok : ret_error;
}


ret_t 
cherokee_get_shell (const char **shell, const char **binary)
{
	char *t1, *t2;

	/* Set the shell path
	 */
#ifdef _WIN32
	*shell = getenv("ComSpec");
#else
	*shell = "/bin/sh";
#endif

	/* Find the binary
	 */
	t1 = strrchr (*shell, '\\');
	t2 = strrchr (*shell, '/');

	t1 = cherokee_max_str (t1, t2);
	if (t1 == NULL) return ret_error;

	*binary = &t1[1];

	return ret_ok;
}


void
cherokee_print_errno (int error, char *format, ...) 
{
	va_list           ap;
	char             *errstr;
	char              err_tmp[ERROR_MAX_BUFSIZE];
	cherokee_buffer_t buffer = CHEROKEE_BUF_INIT;

	errstr = cherokee_strerror_r (error, err_tmp, sizeof(err_tmp));
	if (errstr == NULL)
		errstr = "unknwon error (?)";

	cherokee_buffer_ensure_size (&buffer, 128);
	va_start (ap, format);
	cherokee_buffer_add_va_list (&buffer, format, ap);
	va_end (ap);

	cherokee_buffer_replace_string (&buffer, "${errno}", 8, errstr, strlen(errstr));
	PRINT_MSG_S (buffer.buf);

	cherokee_buffer_mrproper (&buffer);
}


ret_t 
cherokee_mkstemp (cherokee_buffer_t *buffer, int *fd)
{
	int re;

#ifdef _WIN32
	re = cherokee_win32_mkstemp (buffer);
#else
	re = mkstemp (buffer->buf);	
#endif
	if (re < 0) return ret_error;

	*fd = re;
	return ret_ok;
}


void
cherokee_print_wrapped (cherokee_buffer_t *buffer)
{
	char *p;

	p = buffer->buf + TERMINAL_WIDTH;
	for (; p < buffer->buf + buffer->len; p+=75) {
		while (*p != ' ') p--;
		*p = '\n';
	}

	printf ("%s\n", buffer->buf);
	fflush (stdout);
}


ret_t
cherokee_fix_dirpath (cherokee_buffer_t *buf)
{
	while (cherokee_buffer_is_ending(buf, '/')) {
		cherokee_buffer_drop_ending (buf, 1);
	}

	return ret_ok;
}


ret_t
cherokee_mkdir_p (cherokee_buffer_t *path)
{
	int   re;
        char *p;

	if (cherokee_buffer_is_empty (path))
		return ret_ok;

	p = path->buf;
	while (true) {
		p = strchr (p+1, '/');
		if (p == NULL)
			break;

		*p = '\0';
		re = cherokee_mkdir (path->buf, 0700);
		if ((re != 0) && (errno != EEXIST)) {
			PRINT_ERRNO (errno, "Could not mkdir '%s': ${errno}\n", path->buf);
			return ret_error;
		}
		*p = '/';
		
		p++;
		if (p > path->buf + path->len)
			return ret_ok;
	}

	re = cherokee_mkdir (path->buf, 0700);
	if ((re != 0) && (errno != EEXIST)) {
		PRINT_ERRNO (errno, "Could not mkdir '%s': ${errno}\n", path->buf);
		return ret_error;
	}
	
	return ret_ok;
}


ret_t
cherokee_iovec_skip_sent (struct iovec *orig, uint16_t  orig_len,
			  struct iovec *dest, uint16_t *dest_len,
			  size_t sent)
{
	int    i;
 	int    j      = 0;
	size_t total  = 0;

	for (i=0; i<orig_len; i++) {
		if (sent >= total + orig[i].iov_len) {
			/* Already sent */
			total += orig[i].iov_len;

		} else if (j > 0) {
			/* Add the whole thing */
			dest[j].iov_len  = orig[i].iov_len;
			dest[j].iov_base = orig[i].iov_base;
			j++;

		} else {
			/* Add only a piece */
			dest[j].iov_len  = orig[i].iov_len  - (sent - total);
			dest[j].iov_base = orig[i].iov_base + (sent - total);
			j++;
		}
	}

	*dest_len = j;
	return ret_ok;
}


ret_t
cherokee_iovec_was_sent (struct iovec *orig, uint16_t orig_len, size_t sent)
{
	int    i;
	size_t total =0;

	for (i=0; i<orig_len; i++) {
		total += orig[i].iov_len;
		if (total > sent) {
			return ret_eagain;
		}
	}
	
	return ret_ok;
}
