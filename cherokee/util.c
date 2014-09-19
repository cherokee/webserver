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

#include "common-internal.h"
#include "util.h"
#include "logger.h"
#include "bogotime.h"
#include "socket.h"

#include <signal.h>
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

#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
#endif

#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifndef HOST_NAME_MAX
# define HOST_NAME_MAX 255
#endif

#ifndef NSIG
# define NSIG 32
#endif

#define ENTRIES "util"

const char *cherokee_version    = PACKAGE_VERSION;


const char hex2dec_tab[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 00-0F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 10-1F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 20-2F */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,  /* 30-3F */
	0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 40-4F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 50-5F */
	0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 60-6F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 70-7F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 80-8F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 90-9F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* A0-AF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* B0-BF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* C0-CF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* D0-DF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* E0-EF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   /* F0-FF */
};

const char *month[13] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	NULL
};


/* Given an error number (errno) it returns an error string.
 * Parameters "buf" and "bufsize" are passed by caller
 * in order to make the function "thread safe".
 * If the error number is unknown
 * then an "Unknown error nnn" string is returned.
 */
char *
cherokee_strerror_r (int err, char *buf, size_t bufsize)
{
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
	if (getrlimit (RLIMIT_NOFILE, &rlp))
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
	int           re;
	struct rlimit rl;

	rl.rlim_cur = limit;
	rl.rlim_max = limit;

	re = setrlimit (RLIMIT_NOFILE, &rl);
	if (re != 0) {
		return ret_error;
	}

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

char *
strncasestrn (const char *s, size_t slen, const char *find, size_t findlen)
{
	char c;
	char sc;

	if (unlikely (find == NULL) || (findlen == 0))
		return (char *)s;

	if (unlikely (*find == '\0'))
		return (char *)s;

	c = *find;
	find++;
	findlen--;

	do {
		do {
			if (slen-- < 1 || (sc = *s++) == '\0')
				return NULL;
		} while (CHEROKEE_CHAR_TO_LOWER(sc) != CHEROKEE_CHAR_TO_LOWER(c));
		if (findlen > slen) {
			return NULL;
		}
	} while (strncasecmp (s, find, findlen) != 0);

	s--;
	return (char *)s;
}

char *
strncasestr (const char *s, const char *find, size_t slen)
{
	return strncasestrn (s, slen, find, strlen(find));
}



#ifndef HAVE_MALLOC
void *
rpl_malloc (size_t n)
{
	if (unlikely (n == 0))
		n = 1;

	return malloc (n);
}
#endif



/* The following few lines has been copy and pasted from Todd
 * C. Miller <Todd.Miller@courtesan.com> code. BSD licensed.
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
	while (n-- != 0 && *d != '\0') {
		d++;
	}

	dlen = d - dst;
	n = siz - dlen;

	if (n == 0) {
		return(dlen + strlen(s));
	}

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


DIR *
cherokee_opendir (const char *dirname)
{
	DIR *re;

	do {
		re = opendir (dirname);
	} while ((re == NULL) && (errno == EINTR));

	return re;
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

	do {
		errno = 0;
		readdir_r (dirstream, entry);
	} while (errno == EINTR);

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

	do {
		errno = 0;
		ptr = readdir (dirstream);
	} while ((ptr == NULL) && (errno == EINTR));

	if ((ptr == NULL) && (errno != 0))
		ret = errno;

	if (ptr)
		memcpy(entry, ptr, sizeof(*ptr));

	*result = ptr;

	CHEROKEE_MUTEX_UNLOCK (&readdir_mutex);

	return ret;

# endif
#endif
}


int
cherokee_closedir (DIR *dirstream)
{
	int re;

	do {
		re = closedir (dirstream);
	} while ((re < 0) && (errno == EINTR));

	return re;
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
		if (cherokee_stat (path->buf, &st) == -1) {
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
cherokee_estimate_va_length (const char *fmt, va_list ap)
{
	char               *p;
	cuchar_t            ch;
	cllong_t            ll;
	cullong_t           ul;
	cherokee_boolean_t  lflag;
	cherokee_boolean_t  llflag;
	cuint_t             width;
	cuint_t             len = 0;

#define LEN_NUM(var,base)    \
	do {                 \
		var /= base; \
		len++;       \
	} while (var > 0);   \
	len++

	for (;;) {
		width = 0;

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
			len++;
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


ret_t
cherokee_gethostbyname (cherokee_buffer_t *hostname, struct addrinfo **addr)
{
	int              n;
	struct addrinfo  hints;

	/* What we are trying to get
	 */
	memset (&hints, 0, sizeof(struct addrinfo));

	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

#ifdef AI_ADDRCONFIG
	/* Workaround for loopback host addresses:
	 *
	 * If a computer does not have any outgoing IPv6 network
	 * interface, but its loopback network interface supports
	 * IPv6, a getaddrinfo call on "localhost" with AI_ADDRCONFIG
	 * won't return the IPv6 loopback address "::1", because
	 * getaddrinfo() thinks the computer cannot connect to any
	 * IPv6 destination, ignoring the remote vs. local/loopback
	 * distinction.
	 */
	if ((cherokee_buffer_cmp_str (hostname, "::1")       == 0) ||
	    (cherokee_buffer_cmp_str (hostname, "127.0.0.1") == 0) ||
	    (cherokee_buffer_cmp_str (hostname, "localhost")  == 0) ||
	    (cherokee_buffer_cmp_str (hostname, "localhost6") == 0) ||
	    (cherokee_buffer_cmp_str (hostname, "localhost.localdomain")   == 0) ||
	    (cherokee_buffer_cmp_str (hostname, "localhost6.localdomain6") == 0))
	{
		hints.ai_flags = AI_ADDRCONFIG;
	}
#endif

	/* Resolve address
	 */
	n = getaddrinfo (hostname->buf, NULL, &hints, addr);
	if (n < 0) {
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_gethostname (cherokee_buffer_t *buf)
{
	int  re;

#ifdef HAVE_GETHOSTNAME
	char host_name[HOST_NAME_MAX + 1];

	re = gethostname (host_name, HOST_NAME_MAX);
	if (re) {
		return ret_error;
	}

	cherokee_buffer_add (buf, host_name, strlen(host_name));

	return ret_ok;

#elif defined(HAVE_SYS_UTSNAME_H) && defined(HAVE_UNAME)
	struct utsname info;

	re = uname (&info);
	if (re) {
		return ret_error;
	}

	cherokee_buffer_add (buf, info.nodename, sizeof(info.nodename));

	return ret_ok;
#endif

	return ret_error;
}


ret_t
cherokee_fd_set_nodelay (int fd, cherokee_boolean_t enable)
{
	int re;
	int flags = 0;

	/* Disable the Nagle algorithm. This means that segments are
	 * always sent as soon as possible, even if there is only a
	 * small amount of data. When not set, data is buffered until
	 * there is a sufficient amount to send out, thereby avoiding
	 * the frequent sending of small packets, which results in
	 * poor utilization of the network.
	 */

#ifdef FIONBIO
	/* Even though the right thing to do would be to use POSIX's
	 * O_NONBLOCK, we are using FIONBIO here. It requires a single
	 * syscall, while using O_NONBLOCK would require us to call
	 * fcntl(F_GETFL) and fcntl(F_SETFL, O_NONBLOCK)
	 */
	re = ioctl (fd, FIONBIO, &enable);

#else
	/* Use POSIX's O_NONBLOCK
	 */
	flags = fcntl (fd, F_GETFL, 0);
	if (unlikely (flags == -1)) {
		LOG_ERRNO (errno, cherokee_err_warning, CHEROKEE_ERROR_UTIL_F_GETFL, fd);
		return ret_error;
	}

	if (enable)
		BIT_SET (flags, O_NDELAY);
	else
		BIT_UNSET (flags, O_NDELAY);

	re = fcntl (fd, F_SETFL, flags);
#endif

	if (unlikely (re < 0)) {
		LOG_ERRNO (errno, cherokee_err_warning, CHEROKEE_ERROR_UTIL_F_SETFL, fd, flags, "O_NDELAY");
		return ret_error;
	}

	return ret_ok;
}

ret_t
cherokee_fd_set_nonblocking (int fd, cherokee_boolean_t enable)
{
	int re;
	int flags = 0;

	flags = fcntl (fd, F_GETFL, 0);
	if (flags < 0) {
		LOG_ERRNO (errno, cherokee_err_warning, CHEROKEE_ERROR_UTIL_F_GETFL, fd);
		return ret_error;
	}

	if (enable)
		BIT_SET (flags, O_NONBLOCK);
	else
		BIT_UNSET (flags, O_NONBLOCK);

	re = fcntl (fd, F_SETFL, flags);

	if (re < 0) {
		LOG_ERRNO (errno, cherokee_err_warning, CHEROKEE_ERROR_UTIL_F_SETFL, fd, flags, "O_NONBLOCK");
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_fd_set_closexec (int fd)
{
	int re;
	int flags = 0;

	flags = fcntl (fd, F_GETFD, 0);
	if (flags < 0) {
		LOG_ERRNO (errno, cherokee_err_warning, CHEROKEE_ERROR_UTIL_F_GETFD, fd);
		return ret_error;
	}

	BIT_SET (flags, FD_CLOEXEC);

	re = fcntl (fd, F_SETFD, flags);
	if (re < 0) {
		LOG_ERRNO (errno, cherokee_err_warning, CHEROKEE_ERROR_UTIL_F_SETFD, fd, flags, "FD_CLOEXEC");
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_fd_set_reuseaddr (int fd)
{
	int re;
	int on = 1;

	re = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (re != 0) {
		return ret_error;
	}

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

		if (nl == NULL) {
			break;
		}

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

		cherokee_buffer_prepend (path, (char *)"/", 1);
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
#ifdef HAVE_STRUCT_TM_GMTOFF
	struct tm   tm;
	time_t      timestamp;
	static long tz         = 43201; /* 12h+1s: out of range */

	if (unlikely (tz == 43201)) {
		timestamp = time(NULL);
		cherokee_localtime (&timestamp, &tm);
		tz = -tm.tm_gmtoff;
	}
	return &tz;
#else
# ifdef HAVE_INT_TIMEZONE
	return &timezone;
# else
	static long _faked_timezone = 0;
	return &_faked_timezone;
# endif
#endif
}


ret_t
cherokee_parse_query_string (cherokee_buffer_t *qstring, cherokee_avl_t *arguments)
{
	ret_t  ret;
	char  *string;
	char  *token;

	if (cherokee_buffer_is_empty (qstring)) {
		return ret_ok;
	}

	string = qstring->buf;

	while ((token = (char *) strsep(&string, "&")) != NULL) {
		char *equ, *val;

		if (*token == '\0') {
			*token = '&';
			continue;
		}

		if ((equ = strchr(token, '=')) != NULL) {
			cherokee_buffer_t *value;

			val = equ+1;

			ret = cherokee_buffer_new (&value);
			if (unlikely (ret != ret_ok)) {
				return ret;
			}

			cherokee_buffer_add (value, val, strlen(val));

			*equ = '\0';
			cherokee_avl_add_ptr (arguments, token, value);
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


static ret_t
clone_struct_passwd (struct passwd *source, struct passwd *target, char *buf, size_t buflen)
{
	char   *ptr;
	size_t  pw_name_len   = 0;
	size_t  pw_passwd_len = 0;
	size_t  pw_gecos_len  = 0;
	size_t  pw_dir_len    = 0;
	size_t  pw_shell_len  = 0;

	if (source->pw_name)   pw_name_len   = strlen(source->pw_name);
	if (source->pw_passwd) pw_passwd_len = strlen(source->pw_passwd);
	if (source->pw_gecos)  pw_gecos_len  = strlen(source->pw_gecos);
	if (source->pw_dir)    pw_dir_len    = strlen(source->pw_dir);
	if (source->pw_shell)  pw_shell_len  = strlen(source->pw_shell);

	if ((pw_name_len + pw_passwd_len +
	     pw_gecos_len + pw_dir_len + pw_shell_len + 5) >= buflen) {
		/* Buffer overflow.
		 */
		return ret_error;
	}
	memset (buf, 0, buflen);
	ptr = buf;

	target->pw_uid = source->pw_uid;
	target->pw_gid = source->pw_gid;

	if (source->pw_dir) {
		memcpy (ptr, source->pw_dir, pw_dir_len);
		target->pw_dir = ptr;
		ptr += pw_dir_len + 1;
	}

	if (source->pw_passwd) {
		memcpy (ptr, source->pw_passwd, pw_passwd_len);
		target->pw_passwd = ptr;
		ptr += pw_passwd_len + 1;
	}

	if (source->pw_name) {
		memcpy (ptr, source->pw_name, pw_name_len);
		target->pw_name = ptr;
		ptr += pw_name_len + 1;
	}

	if (source->pw_gecos) {
		memcpy (ptr, source->pw_gecos, pw_gecos_len);
		target->pw_gecos = ptr;
		ptr += pw_gecos_len + 1;
	}

	if (source->pw_shell) {
		memcpy (ptr, source->pw_shell, pw_shell_len);
		target->pw_shell = ptr;
		ptr += pw_shell_len + 1;
	}

	return ret_ok;
}


static ret_t
clone_struct_group (struct group *source, struct group *target, char *buf, size_t buflen)
{
	char   *ptr;
	size_t  gr_name_len   = 0;
	size_t  gr_passwd_len = 0;

	if (source->gr_name)   gr_name_len   = strlen(source->gr_name);
	if (source->gr_passwd) gr_passwd_len = strlen(source->gr_passwd);

	if ((gr_name_len + gr_passwd_len) >= buflen) {
		/* Buffer overflow */
		return ret_error;
	}

	memset (buf, 0, buflen);
	ptr = buf;

	target->gr_gid = source->gr_gid;

	if (source->gr_name) {
		memcpy (ptr, source->gr_name, gr_name_len);
		target->gr_name = ptr;
		ptr += gr_name_len + 1;
	}

	if (source->gr_passwd) {
		memcpy (ptr, source->gr_passwd, gr_passwd_len);
		target->gr_passwd = ptr;
		ptr += gr_passwd_len + 1;
	}

	/* TODO: Duplicate '**gr_mem' as well
	 */

	return ret_ok;
}


#if defined(HAVE_PTHREAD) && !defined(HAVE_GETPWNAM_R)
static pthread_mutex_t __global_getpwnam_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

ret_t
cherokee_getpwnam (const char *name, struct passwd *pwbuf, char *buf, size_t buflen)
{
#ifndef HAVE_GETPWNAM_R
	ret_t          ret;
	struct passwd *tmp;

	CHEROKEE_MUTEX_LOCK (&__global_getpwnam_mutex);

	do {
		tmp = getpwnam (name);
	} while ((tmp == NULL) && (errno == EINTR));

	if (tmp == NULL) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getpwnam_mutex);
		return ret_error;
	}

	ret = clone_struct_passwd (tmp, pwbuf, buf, buflen);
	if (ret != ret_ok) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getpwnam_mutex);
		return ret_error;
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
	 *                 struct passwd  *pwbuf,
	 *                 char           *buf,
	 *                 size_t          buflen,
	 *                 struct passwd **pwbufp);
	 */
	do {
		re = getpwnam_r (name, pwbuf, buf, buflen, &tmp);
	} while ((re != 0) && (errno == EINTR));

	if ((re != 0) || (tmp == NULL))
		return ret_error;

	return ret_ok;

#elif HAVE_GETPWNAM_R_4
	struct passwd * result;

#ifdef _POSIX_PTHREAD_SEMANTICS
	int re;

	do {
		re = getpwnam_r (name, pwbuf, buf, buflen, &result);
	} while ((re != 0) && (errno == EINTR));

	if (re != 0)
		return ret_error;
#else
	do {
		result = getpwnam_r (name, pwbuf, buf, buflen);
	} while ((result == NULL) && (errno == EINTR));

	if (result == NULL)
		return ret_error;
#endif

	return ret_ok;
#endif

	return ret_no_sys;
}


#if defined(HAVE_PTHREAD) && !defined(HAVE_GETPWUID_R)
static pthread_mutex_t __global_getpwuid_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

ret_t
cherokee_getpwuid (uid_t uid, struct passwd *pwbuf, char *buf, size_t buflen)
{
#ifdef HAVE_GETPWUID_R_5
	int            re;
	struct passwd *result = NULL;

	/* Linux:
	 *  int getpwuid_r (uid_t uid, struct passwd *pwd, char *buf,    size_t buflen,  struct passwd **result);
	 * MacOS X:
	 *  int getpwuid_r (uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result);
	 */
	do {
		re = getpwuid_r (uid, pwbuf, buf, buflen, &result);
	} while ((re != 0) && (errno == EINTR));

	if ((re != 0) || (result == NULL)) {
		return ret_error;
	}

	return ret_ok;

#elif HAVE_GETPWUID_R_4
	struct passwd *result;

	/* Solaris:
	 * struct passwd *getpwuid_r (uid_t uid, struct passwd *pwd, char *buffer, int  buflen);
	 */
	do {
		result = getpwuid_r (uid, pwbuf, buf, buflen);
	} while ((result == NULL) && (errno == EINTR));

	if (result == NULL) {
		return ret_error;
	}

	return ret_ok;
#else
	ret_t          ret;
	struct passwd *tmp;

	/* struct passwd *getpwuid (uid_t uid);
	 */
	CHEROKEE_MUTEX_LOCK (&__global_getpwuid_mutex);

	do {
		tmp = getpwuid (uid);
	} while ((tmp == NULL) && (errno == EINTR));

	if (tmp == NULL) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getpwuid_mutex);
		return ret_error;
	}

	ret = clone_struct_passwd (tmp, pwbuf, buf, buflen);
	if (ret != ret_ok) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getpwuid_mutex);
		return ret_error;
	}

	CHEROKEE_MUTEX_UNLOCK (&__global_getpwuid_mutex);
	return ret_ok;
#endif

	return ret_no_sys;
}


ret_t
cherokee_getpwnam_uid (const char *name, struct passwd *pwbuf, char *buf, size_t buflen)
{
	ret_t ret;
	long  tmp_uid;

	memset (buf, 0, buflen);
	memset (pwbuf, 0, sizeof(struct passwd));

	ret = cherokee_getpwnam (name, pwbuf, buf, buflen);
	if ((ret == ret_ok) && (pwbuf->pw_dir)) {
		return ret_ok;
	}

	errno   = 0;
	tmp_uid = strtol (name, NULL, 10);
	if (errno != 0) {
		return ret_error;
	}

	ret = cherokee_getpwuid ((uid_t)tmp_uid, pwbuf, buf, buflen);
	if ((ret != ret_ok) || (pwbuf->pw_dir == NULL)) {
		return ret_error;
	}

	return ret_ok;
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

	do {
		tmp = getgrnam (name);
	} while ((tmp == NULL) && (errno == EINTR));

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

	do {
		re = getgrnam_r (name, grbuf, buf, buflen, &tmp);
	} while ((re != 0) && (errno == EINTR));

	if (re != 0)
		return ret_error;

	return ret_ok;

#elif HAVE_GETGRNAM_R_4
	struct group  *result;

#ifdef _POSIX_PTHREAD_SEMANTICS
	int re;
	do {
		re = getgrnam_r (name, grbuf, buf, buflen, &result);
	} while ((re != 0) && (errno == EINTR));

	if (re != 0)
		return ret_error;
#else
	do {
		result = getgrnam_r (name, grbuf, buf, buflen);
	} while ((result == NULL) && (errno == EINTR));

	if (result == NULL)
		return ret_error;
#endif

	return ret_ok;
#endif

	return ret_no_sys;
}


#if defined(HAVE_PTHREAD) && !defined(HAVE_GETGRGID_R)
static pthread_mutex_t __global_getgrgid_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

ret_t
cherokee_getgrgid (gid_t gid, struct group *grbuf, char *buf, size_t buflen)
{
#ifdef HAVE_GETGRGID_R_5
	int            re;
	struct group *result = NULL;

	/* Linux:
	 *  int getgrgid_r (gid_t gid, struct group *grp, char *buf, size_t buflen, struct group **result);
	 * MacOS X:
	 *  int getgrgid_r (gid_t gid, struct group *grp, char *buffer, size_t bufsize, struct group **result);
	 */
	do {
		re = getgrgid_r (gid, grbuf, buf, buflen, &result);
	} while ((re != 0) && (errno == EINTR));

	if ((re != 0) || (result == NULL)) {
		return ret_error;
	}

	return ret_ok;

#elif HAVE_GETGRGID_R_4
	struct group *result = NULL;

	/* Solaris:
	 *  struct group *getgrgid_r (gid_t gid, struct group *grp, char *buffer, int bufsize);
	 */
	do {
		result = getgrgid_r (gid, grbuf, buf, buflen);
	} while ((result == NULL) && (errno == EINTR));

	if (result == NULL) {
		return ret_error;
	}

	return ret_ok;
#else
	ret_t         ret;
	struct group *tmp;

	CHEROKEE_MUTEX_LOCK (&__global_getgrgid_mutex);

	do {
		tmp = getgrgid (gid);
	} while ((tmp == NULL) && (errno == EINTR));

	if (tmp == NULL) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getgrgid_mutex);
		return ret_error;
	}

	ret = clone_struct_group (tmp, grbuf, buf, buflen);
	if (ret != ret_ok) {
		CHEROKEE_MUTEX_UNLOCK (&__global_getgrgid_mutex);
		return ret_error;
	}

	CHEROKEE_MUTEX_UNLOCK (&__global_getgrgid_mutex);
	return ret_ok;
#endif

	return ret_no_sys;
}


ret_t
cherokee_getgrnam_gid (const char *name, struct group *grbuf, char *buf, size_t buflen)
{
	ret_t ret;
	long  tmp_gid;

	memset (buf, 0, buflen);
	memset (grbuf, 0, sizeof(struct group));

	ret = cherokee_getgrnam (name, grbuf, buf, buflen);
	if ((ret == ret_ok) && (grbuf->gr_name)) {
		return ret_ok;
	}

	errno   = 0;
	tmp_gid = strtol (name, NULL, 10);
	if (errno != 0) {
		return ret_error;
	}

	ret = cherokee_getgrgid ((gid_t)tmp_gid, grbuf, buf, buflen);
	if ((ret != ret_ok) || (grbuf->gr_name == NULL)) {
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_fd_close (int fd)
{
	int re;

	if (unlikely (fd < 0)) {
		return ret_error;
	}

	re = close (fd);

	TRACE (ENTRIES",close_fd", "fd=%d re=%d\n", fd, re);
	return (re == 0) ? ret_ok : ret_error;
}


ret_t
cherokee_get_shell (const char **shell, const char **binary)
{
	char *t1, *t2;

	/* Set the shell path
	 */
	*shell = "/bin/sh";

	/* Find the binary
	 */
	t1 = strrchr (*shell, '\\');
	t2 = strrchr (*shell, '/');

	t1 = cherokee_max_str (t1, t2);
	if (t1 == NULL) return ret_error;

	*binary = &t1[1];

	return ret_ok;
}


ret_t
cherokee_buf_add_bogonow (cherokee_buffer_t  *buf,
                          cherokee_boolean_t  update)
{
	if (update) {
		cherokee_bogotime_try_update();
	}

	cherokee_buffer_add_va (buf, "%02d/%02d/%d %02d:%02d:%02d.%03d",
	                        cherokee_bogonow_tmloc.tm_mday,
	                        cherokee_bogonow_tmloc.tm_mon + 1,
	                        cherokee_bogonow_tmloc.tm_year + 1900,
	                        cherokee_bogonow_tmloc.tm_hour,
	                        cherokee_bogonow_tmloc.tm_min,
	                        cherokee_bogonow_tmloc.tm_sec,
	                        cherokee_bogonow_tv.tv_usec / 1000);
	return ret_ok;
}


ret_t
cherokee_buf_add_backtrace (cherokee_buffer_t *buf,
                            int                n_skip,
                            const char        *new_line,
                            const char        *line_pre)
{
#if HAVE_BACKTRACE
	void    *array[128];
	size_t   size;
	char   **strings;
	size_t   i;
	int      line_pre_len;
	int      new_line_len;

	line_pre_len = strlen (line_pre);
	new_line_len = strlen (new_line);

	size = backtrace (array, 128);
	strings = backtrace_symbols (array, size);

	for (i=n_skip; i < size; i++) {
		if (line_pre_len > 0) {
			cherokee_buffer_add (buf, line_pre, line_pre_len);
		}
		cherokee_buffer_add (buf, strings[i], strlen(strings[i]));
		cherokee_buffer_add (buf, new_line, new_line_len);
	}

	free (strings);
	return ret_ok;
#else
	return ret_no_sys;
#endif
}


ret_t
cherokee_mkstemp (cherokee_buffer_t *buffer, int *fd)
{
	int re;

	re = mkstemp (buffer->buf);
    
	if (re < 0) return ret_error;

	*fd = re;
	return ret_ok;
}


ret_t
cherokee_mkdtemp (char *template)
{
	char *re;

	re = mkdtemp (template);
	if (unlikely (re == NULL)) {
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_fix_dirpath (cherokee_buffer_t *buf)
{
	while ((buf->len > 1) &&
	       (cherokee_buffer_is_ending (buf, '/')))
	{
		cherokee_buffer_drop_ending (buf, 1);
	}

	return ret_ok;
}


ret_t
cherokee_mkdir_p (cherokee_buffer_t *path, int mode)
{
	int          re;
	char        *p;
	int          err;
	struct stat  foo;

	/* There is no directory
	 */
	if (cherokee_buffer_is_empty (path)) {
		return ret_ok;
	}

	/* Check whether the directory exists
	 */
	re = cherokee_stat (path->buf, &foo);
	if (re == 0) {
		return ret_ok;
	}

	/* Create the directory tree
	 */
	p = path->buf;
	while (true) {
		p = strchr (p+1, '/');
		if (p == NULL)
			break;

		*p = '\0';

		re = cherokee_stat (path->buf, &foo);
		if (re != 0) {
			re = cherokee_mkdir (path->buf, mode);
			if ((re != 0) && (errno != EEXIST)) {
				err = errno;
				*p = '/';

				LOG_ERRNO (err, cherokee_err_error, CHEROKEE_ERROR_UTIL_MKDIR, path->buf, getuid());
				return ret_error;
			}
		}

		*p = '/';

		p++;
		if (p > path->buf + path->len)
			return ret_ok;
	}

	re = cherokee_mkdir (path->buf, mode);
	if ((re != 0) && (errno != EEXIST)) {
		err = errno;

		LOG_ERRNO (err, cherokee_err_error, CHEROKEE_ERROR_UTIL_MKDIR, path->buf, getuid());
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_mkdir_p_perm (cherokee_buffer_t *dir_path,
                       int                create_mode,
                       int                ensure_perm)
{
	int         re;
	ret_t       ret;
	struct stat foo;

	/* Does it exist?
	 */
	re = cherokee_stat (dir_path->buf, &foo);
	if (re != 0) {
		/* Create the directory
		 */
		ret = cherokee_mkdir_p (dir_path, create_mode);
		if (ret != ret_ok) {
			return ret_error;
		}
	}

	/* Check permissions
	 */
	re = cherokee_access (dir_path->buf, ensure_perm);
	if (re != 0) {
		return ret_deny;
	}

	return ret_ok;
}


ret_t
cherokee_rm_rf (cherokee_buffer_t *path,
                uid_t              only_uid)
{
	int               re;
	DIR              *d;
	struct dirent    *entry;
	char              entry_buf[512];
	struct stat       info;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	/* Remove the directory contents
	 */
	d = cherokee_opendir (path->buf);
	if (d == NULL) {
		return ret_ok;
	}

	while (true) {
		re = cherokee_readdir (d, (struct dirent *)entry_buf, &entry);
		if ((re != 0) || (entry == NULL))
			break;

		if (!strncmp (entry->d_name, ".",  1)) continue;
		if (!strncmp (entry->d_name, "..", 2)) continue;

		cherokee_buffer_clean      (&tmp);
		cherokee_buffer_add_buffer (&tmp, path);
		cherokee_buffer_add_char   (&tmp, '/');
		cherokee_buffer_add        (&tmp, entry->d_name, strlen(entry->d_name));

		if (only_uid != -1) {
			re = cherokee_stat (tmp.buf, &info);
			if (re != 0) continue;

			if (info.st_uid != only_uid)
				continue;
		}

		if (S_ISDIR (info.st_mode)) {
			cherokee_rm_rf (&tmp, only_uid);
			TRACE (ENTRIES, "Removing cache dir: %s\n", tmp.buf, re);
		} else if (S_ISREG (info.st_mode)) {
			re = unlink (tmp.buf);
			TRACE (ENTRIES, "Removing cache file: %s, re=%d\n", tmp.buf, re);
		}
	}

	cherokee_closedir (d);

	/* It should be empty by now
	 */
	re = rmdir (path->buf);
	TRACE (ENTRIES, "Removing main vserver cache dir: %s, re=%d\n", path->buf, re);

	/* Clean up
	 */
	cherokee_buffer_mrproper (&tmp);
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
			dest[j].iov_base = ((char *)orig[i].iov_base) + (sent - total);
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


ret_t
cherokee_io_stat (cherokee_iocache_t        *iocache,
                  cherokee_buffer_t         *path,
                  cherokee_boolean_t         useit,
                  struct stat               *nocache_info,
                  cherokee_iocache_entry_t **io_entry,
                  struct stat              **info)
{
	ret_t ret;
	int   re  = -1;

	/* I/O cache
	 */
	if ((useit) &&
	    (iocache != NULL))
	{
		ret = cherokee_iocache_autoget (iocache, path, iocache_stat, io_entry);
		TRACE (ENTRIES, "%s, use_iocache=1 ret=%d\n", path->buf, ret);

		switch (ret) {
		case ret_ok:
		case ret_ok_and_sent:
			*info = &(*io_entry)->state;
			return (*io_entry)->state_ret;

		case ret_no_sys:
			goto without;

		case ret_deny:
		case ret_not_found:
			return ret;
		default:
			return ret_error;
		}
	}

	/* Without cache
	 */
without:
	re = cherokee_stat (path->buf, nocache_info);
	TRACE (ENTRIES, "%s, use_iocache=0 re=%d\n", path->buf, re);

	if (re >= 0) {
		*info = nocache_info;
		return ret_ok;
	}

	switch (errno) {
	case ENOENT:
	case ENOTDIR:
		return ret_not_found;
	case EACCES:
		return ret_deny;
	default:
		return ret_error;
	}

	return ret_error;
}


char *
cherokee_header_get_next_line (char *string)
{
	char *end1;
	char *end2 = string;

	/* RFC states that EOL should be made by CRLF only, but some
	 * old clients (HTTP 0.9 and a few HTTP/1.0 robots) may send
	 * LF only as EOL, so try to catch that case too (of course CR
	 * only is ignored); anyway leading spaces after a LF or CRLF
	 * means that the new line is the continuation of previous
	 * line (first line excluded).
	 */
	do {
		end1 = end2;
		end2 = strchr (end1, CHR_LF);

		if (unlikely (end2 == NULL))
			return NULL;

		end1 = end2;
		if (likely (end2 != string && *(end1 - 1) == CHR_CR))
			--end1;

		++end2;
	} while (*end2 == CHR_SP || *end2 == CHR_HT);

	return end1;
}

ret_t
cherokee_header_del_entry (cherokee_buffer_t *header,
                           const char        *header_name,
                           int                header_name_len)
{
	char                        *end;
	char                        *begin;
	const char                  *header_end;

	begin      = header->buf;
	header_end = header->buf + header->len;

	while ((begin < header_end)) {
		end = cherokee_header_get_next_line (begin);
		if (end == NULL) {
			break;
		}

		/* Is it the header? */
		if (strncasecmp (begin, header_name, header_name_len) == 0) {
			while ((*end == CHR_CR) || (*end == CHR_LF))
				end++;

			cherokee_buffer_remove_chunk (header, begin - header->buf, end - begin);
			return ret_ok;
		}

		/* Next line */
		while ((*end == CHR_CR) || (*end == CHR_LF))
			end++;
		begin = end;
	}

	return ret_not_found;
}


#ifndef HAVE_STRNSTR
char *
strnstr (const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}
#endif


ret_t
cherokee_find_header_end_cstr (char      *c_str,
                               cint_t     c_len,
                               char     **end,
                               cuint_t   *sep_len)
{
	char *p;
	char *fin;
	char *begin;
	int   cr_n, lf_n;

	if ((c_str == NULL) || (c_len <= 0))
		return ret_not_found;

	p   = c_str;
	fin = c_str + MIN(c_len, MAX_HEADER_LEN);

	while (p < fin) {
		if ((*p == CHR_CR) || (*p == CHR_LF)) {
			cr_n  = 0;
			lf_n  = 0;
			begin = p;

			/* Valid scenarios:
			 * CR_n: [CRLF_CRLF] 0, 1, 1, 2, 2  | [LF_LF] 0, 0
			 * LF_n:             0, 0, 1, 1, 2  |         1, 2
			 *
			 * so, the two forbidden situations are:
			 * CR_n: 1, 2
			 * LF_n: 2, 0
			 */
			while (p < fin) {
				if (*p == CHR_LF) {
					lf_n++;
					if (lf_n == 2) {
						*end     = begin;
						*sep_len = (p - begin) + 1;
						return ret_ok;
					}

				} else if (*p == CHR_CR) {
					cr_n++;

				} else {
					break;
				}

				if (unlikely (((cr_n == 1) && (lf_n == 2)) ||
					      ((cr_n == 2) && (lf_n == 0))))
				{
					return ret_error;
				}

				p++;
			}
		}

		p++;
	}

	return ret_not_found;
}


ret_t
cherokee_find_header_end (cherokee_buffer_t  *buf,
                          char              **end,
                          cuint_t            *sep_len)
{
	return cherokee_find_header_end_cstr (buf->buf, buf->len, end, sep_len);
}


ret_t
cherokee_ntop (int family, struct sockaddr *addr, char *dst, size_t cnt)
{
	const char *str = NULL;
	errno = EAFNOSUPPORT;

	/* Only old systems without inet_ntop() function
	 */
#ifndef HAVE_INET_NTOP
	{
		str = inet_ntoa (((struct sockaddr_in *)addr)->sin_addr);
		memcpy(dst, str, strlen(str));

		return ret_ok;
	}
#else
# ifdef HAVE_IPV6
	if (family == AF_INET6) {
		struct in6_addr *addr6 = &(((struct sockaddr_in6 *)addr)->sin6_addr);

		if (IN6_IS_ADDR_V4MAPPED (addr6) ||
		    IN6_IS_ADDR_V4COMPAT (addr6))
		{
			const void *p = &(addr6)->s6_addr[12];

			str = inet_ntop (AF_INET, p, dst, cnt);
			if (str == NULL) {
				goto error;
			}
		} else {
			str = (char *) inet_ntop (AF_INET6, addr6, dst, cnt);
			if (str == NULL) {
				goto error;
			}
		}
	} else
# endif
	{
		struct in_addr *addr4 = &((struct sockaddr_in *)addr)->sin_addr;

		str = inet_ntop (AF_INET, addr4, dst, cnt);
		if (str == NULL) {
			goto error;
		}
	}
#endif

	return ret_ok;

error:
	dst[0] = '\0';
	return ret_error;
}


ret_t
cherokee_tmp_dir_copy (cherokee_buffer_t *buffer)
{
	char *p;

	/* Read a custom Cherokee variable
	 */
	p = getenv("CHEROKEE_TMPDIR");
	if (p != NULL) {
		cherokee_buffer_add (buffer, p, strlen(p));
		return ret_ok;
	}

	/* Read the system variable
	 */
	p = getenv("TMPDIR");

	if (p != NULL) {
		cherokee_buffer_add (buffer, p, strlen(p));
		return ret_ok;
	}

	/* Since everything has failed, let's go for whatever autoconf
	 * detected at compilation time.
	 */
	cherokee_buffer_add_str (buffer, TMPDIR);
	return ret_ok;
}


ret_t
cherokee_parse_host (cherokee_buffer_t  *buf,
                     cherokee_buffer_t  *host,
                     cuint_t            *port)
{
	char *p;
	char *colon;

	colon = strchr (buf->buf, ':');
	if (colon == NULL) {
		/* Host
		 */
		cherokee_buffer_add_buffer (host, buf);
		return ret_ok;
	}

	p = strchr (colon+1, ':');
	if (p == NULL) {
		/* Host:port
		 */
		*port = atoi (colon+1);
		cherokee_buffer_add (host, buf->buf, colon - buf->buf);
		return ret_ok;
	}

#ifdef HAVE_IPV6
	if (buf->buf[0] != '[') {
		/* IPv6
		 */
		cherokee_buffer_add_buffer (host, buf);
		return ret_ok;
	}

	/* [IPv6]:port
	 */
	colon = strrchr(buf->buf, ']');
	if (unlikely (colon == NULL))
		return ret_error;
	if (unlikely (colon[1] != ':'))
		return ret_error;

	colon += 1;
	*port = atoi(colon+1);
	cherokee_buffer_add (host, buf->buf+1, (colon-1)-(buf->buf+1));
	return ret_ok;
#endif

	return ret_error;
}


int
cherokee_string_is_ipv6 (cherokee_buffer_t *ip)
{
	cuint_t i;
	cuint_t colons = 0;

	for (i=0; i<ip->len; i++) {
		if (ip->buf[i] == ':') {
			colons += 1;
			if (colons == 2)
				return 1;
		}
	}

	return 0;
}


ret_t
cherokee_find_exec_in_path (const char        *bin_name,
                            cherokee_buffer_t *fullpath)
{
	int   re;
	char *p, *q;
	char *path;

	p = getenv("PATH");
	if (p == NULL) {
		return ret_not_found;
	}

	path = strdup(p);
	if (unlikely (path == NULL)) {
		return ret_nomem;
	}

	p = path;
	do {
		q = strchr (p, ':');
		if (q) {
			*q = '\0';
		}

		cherokee_buffer_clean   (fullpath);
		cherokee_buffer_add     (fullpath, p, strlen(p));
		cherokee_buffer_add_str (fullpath, "/");
		cherokee_buffer_add     (fullpath, bin_name, strlen(bin_name));

		re = cherokee_access (fullpath->buf, X_OK);
		if (re == 0) {
			free (path);
			return ret_ok;
		}

		p = q + 1;
	} while(q);

	free (path);
	return ret_not_found;
}


ret_t
cherokee_atoi (const char *str, int *ret_value)
{
	int tmp;

	errno = 0;
	tmp = strtol (str, NULL, 10);
	if (errno != 0) {
		return ret_error;
	}

	*ret_value = tmp;
	return ret_ok;
}


ret_t
cherokee_atou (const char *str, cuint_t *ret_value)
{
	cuint_t tmp;

	errno = 0;
	tmp = strtoul (str, NULL, 10);
	if (errno != 0) {
		return ret_error;
	}

	*ret_value = tmp;
	return ret_ok;
}


ret_t
cherokee_atob (const char *str, cherokee_boolean_t *ret_value)
{
	ret_t ret;
	int   tmp;

	ret = cherokee_atoi (str, &tmp);
	if (ret != ret_ok) {
		return ret;
	}

	*ret_value = !!tmp;
	return ret_ok;
}


ret_t
cherokee_copy_local_address (void              *sock,
                             cherokee_buffer_t *buf)
{
	int                 re;
	cherokee_sockaddr_t my_address;
	cuint_t             my_address_len = 0;
	char                ip_str[CHE_INET_ADDRSTRLEN+1];
	cherokee_socket_t  *socket = SOCKET(sock);

	/* Get the address
	 */
	my_address_len = sizeof(my_address);
	re = getsockname (SOCKET_FD(socket), (struct sockaddr *)&my_address, &my_address_len);
	if (re != 0) {
		return ret_error;
	}

	/* Build the string
	 */
	cherokee_ntop (my_address.sa_in.sin_family,
		       (struct sockaddr *) &my_address,
		       ip_str, sizeof(ip_str)-1);

	/* Copy it to the buffer
	 */
	cherokee_buffer_add (buf, ip_str, strlen(ip_str));
	return ret_ok;
}


int
cherokee_stat (const char *restrict path, struct stat *buf)
{
	int re;

	do {
		re = stat (path, buf);
	} while ((re == -1) && (errno == EINTR));

	return re;
}

int
cherokee_lstat (const char *restrict path, struct stat *buf)
{
	int re;

	do {
		re = lstat (path, buf);
	} while ((re == -1) && (errno == EINTR));

	return re;
}

int
cherokee_fstat (int filedes, struct stat *buf)
{
	int re;

	do {
		re = fstat (filedes, buf);
	} while ((re == -1) && (errno == EINTR));

	return re;
}

int
cherokee_access (const char *pathname, int mode)
{
	int re;

	do {
		re = access (pathname, mode);
	} while ((re == -1) && (errno == EINTR));

	return re;
}

ret_t
cherokee_wait_pid (int pid, int *retcode)
{
	int re;
	int exitcode;

	TRACE(ENTRIES",signal", "Handling SIGCHLD, waiting PID %d\n", pid);

	while (true) {
		re = waitpid (pid, &exitcode, 0);
		if (re > 0) {
			if (WIFEXITED(exitcode) && (retcode != NULL)) {
				*retcode = WEXITSTATUS(exitcode);
			}
			return ret_ok;
		}

		else if (errno == ECHILD) {
			return ret_not_found;
		}

		else if (errno == EINTR) {
			continue;

		} else {
			PRINT_ERROR ("ERROR: waiting PID %d, error %d\n", pid, errno);
			return ret_error;
		}
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
cherokee_reset_signals (void)
{
	int              i;
	struct sigaction sig_action;

	/* Reset signal handlers */
	sig_action.sa_handler = SIG_DFL;
	sig_action.sa_flags   = 0;
	sigemptyset (&sig_action.sa_mask);

	for (i = 1; i < NSIG; i++) {
		sigaction (i, &sig_action, NULL);
	}

	return ret_ok;
}


int
cherokee_unlink (const char *path)
{
	int re;

	do {
		re = unlink (path);
	} while ((re < 0) && (errno == EINTR));

	return re;
}


int
cherokee_open (const char *path, int oflag, int mode)
{
	int re;

	do {
		re = open (path, oflag, mode);
	} while ((re < 0) && (errno == EINTR));

	return re;
}


ret_t
cherokee_mkdir (const char *path, int mode)
{
	int re;

	do {
		re = mkdir (path, mode);
	} while ((re < 0) && (errno == EINTR));

	return re;
}


int
cherokee_pipe (int fildes[2])
{
	int re;

	do {
		re = pipe (fildes);
	} while ((re < 0) && (errno == EINTR));

	return re;
}

int cherokee_socketpair (int fildes[2], cherokee_boolean_t stream)
{
	int re;

	do {
		re = socketpair (AF_UNIX, stream ? SOCK_STREAM : SOCK_DGRAM,
				 0, fildes);
	} while ((re < 0) && (errno == EINTR));

	return re;
}


void
cherokee_random_seed (void)
{
#ifdef HAVE_SRANDOMDEV
	srandomdev();
#else
	int      fd;
	ssize_t  re;
	unsigned seed;

	/* Open device
	 */
	fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1) {
		fd = open("/dev/random", O_RDONLY);
	}

	/* Read seed
	 */
	if (fd != -1) {
		do {
			re = read (fd, &seed, sizeof(seed));
		} while ((re == -1) && (errno == EINTR));

		cherokee_fd_close(fd);

		if (re == sizeof(seed))
			goto out;
	}

	/* Home-made seed
	 */
	cherokee_bogotime_update();

	seed = cherokee_bogonow_tv.tv_usec;
	if (cherokee_bogonow_tv.tv_usec & 0xFF)
		seed *= (cherokee_bogonow_tv.tv_usec & 0xFF);

out:
	/* Set the seed
	 */
# if HAVE_SRANDOM
	srandom (seed);
# else
	srand (seed);
# endif
#endif
}


long
cherokee_random (void)
{
#ifdef HAVE_RANDOM
	return random();
#else
	return rand();
#endif
}
