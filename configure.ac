dnl -*- mode: m4-mode -*-
dnl Process this file with autoconf to produce a configure script.

AC_COPYRIGHT([
Copyright (C) 2001-2014 Alvaro Lopez Ortega

This program is free software; you can redistribute it and/or
modify it under the terms of version 2 of the GNU General Public
License as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
])

dnl require autoconf 2.68
AC_PREREQ([2.68])

dnl Version
m4_define([cherokee_major_version], [1])
m4_define([cherokee_minor_version], [2])
m4_define([cherokee_micro_version], [104])
m4_define([cherokee_version], m4_format('%s.%s.%s', cherokee_major_version, cherokee_minor_version, cherokee_micro_version))

dnl Init autoconf and automake
AC_INIT([cherokee], [cherokee_version], [http://bugs.cherokee-project.com/], [cherokee])
AC_CONFIG_SRCDIR([cherokee/server.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([no-define])

dnl Define version
AC_DEFINE(PACKAGE_MAJOR_VERSION, "cherokee_major_version", [Version string])
AC_DEFINE(PACKAGE_MINOR_VERSION, "cherokee_minor_version", [Version string])
AC_DEFINE(PACKAGE_MICRO_VERSION, "cherokee_micro_version", [Version string])

PACKAGE_MAJOR_VERSION="cherokee_major_version"
PACKAGE_MINOR_VERSION="cherokee_minor_version"
PACKAGE_MICRO_VERSION="cherokee_micro_version"
AC_SUBST(PACKAGE_MAJOR_VERSION)
AC_SUBST(PACKAGE_MINOR_VERSION)
AC_SUBST(PACKAGE_MICRO_VERSION)

dnl Define library soname
CHEROKEE_AGE=0
CHEROKEE_CURRENT=0
CHEROKEE_REVISION=1
AC_SUBST(CHEROKEE_AGE)
AC_SUBST(CHEROKEE_CURRENT)
AC_SUBST(CHEROKEE_REVISION)

AC_DEFINE_UNQUOTED(CHEROKEE_CONFIG_ARGS, "$ac_configure_args", [configure arguments])

dnl Check the SVN revision
AC_ARG_ENABLE(beta, AC_HELP_STRING([--enable-beta], [Enable beta development]),
		    is_beta="$enableval", is_beta="no")

if test x"$is_beta" = "xyes"; then
  dnl Figure revision
  today=`date +'%y%m%d'`
  git_hash=`git log -1 --pretty=tformat:%h`

  PACKAGE_PATCH_VERSION="b${today}_${git_hash}"

  dnl Redefine versions
  PACKAGE_VERSION="$PACKAGE_MAJOR_VERSION.$PACKAGE_MINOR_VERSION.$PACKAGE_MICRO_VERSION$PACKAGE_PATCH_VERSION"
  AC_DEFINE_UNQUOTED(PACKAGE_VERSION, "${PACKAGE_VERSION}", [Package version])

  VERSION="$PACKAGE_MAJOR_VERSION.$PACKAGE_MINOR_VERSION.$PACKAGE_MICRO_VERSION$PACKAGE_PATCH_VERSION"
  AC_DEFINE(VERSION, $VERSION, [Software version])
else

  PACKAGE_PATCH_VERSION=""
fi

dnl Path version
AC_SUBST(PACKAGE_PATCH_VERSION)
AC_DEFINE_UNQUOTED(PACKAGE_PATCH_VERSION, "${PACKAGE_PATCH_VERSION}", [Version string])

dnl Initial CFLAGS
AC_MSG_CHECKING(initial CFLAGS)
AC_MSG_RESULT($CFLAGS)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl Check for CPU / vendor / OS
AC_CANONICAL_HOST

os_string="UNIX"
so_suffix=so
mod_suffix=$so_suffix

AC_MSG_CHECKING([host platform characteristics])
	case "$host" in
	*-*-linux*)
          libdl="-ldl"
          ;;
	*-*-solaris*)
	     AC_DEFINE(SOLARIS, 1, [It is Solaris])
		setenv_is_threadsafe="yes"
          libdl="-ldl"
          ;;
	*-*-hpux*)
          libdl="-ldl"
          ;;
	*-*-aix*)
          libdl="-ldl"
	     ;;
     *-*-irix6*)
          AC_DEFINE(IRIX, 1, [It is SGI Irix])
          setenv_is_threadsafe="yes"
          libdl="-ldl"
          ;;
	*-*-*freebsd*|*-*-*openbsd*|*-*-*netbsd*|*-*-*dragonfly*)
          libdl=
          ;;
	*-*-darwin*)
		so_suffix=dylib
		mod_suffix=so
          libdl="-ldl"
          ;;
	*)
		AC_MSG_WARN([*** Please add $host to configure.ac checks!])
          libdl="-ldl"
          ;;
esac
AC_MSG_RESULT(ok)

AC_MSG_CHECKING([L2 cache line size])
case "$host_cpu" in
	i386|i486)
		CPU_CACHE_LINE=32
	;;
	i586|i686)
		CPU_CACHE_LINE=64
	;;
	i786)
		CPU_CACHE_LINE=128
	;;
	athlon)
		CPU_CACHE_LINE=64
	;;
	x86_64)
		CPU_CACHE_LINE=64
	;;
	sparc*)
		CPU_CACHE_LINE=64
	;;
	powerpc)
		CPU_CACHE_LINE=128
	;;
	*)
		CPU_CACHE_LINE=128
	;;
esac
AC_DEFINE_UNQUOTED(CPU_CACHE_LINE, $CPU_CACHE_LINE, [L2 cache line size])
AC_MSG_RESULT($CPU_CACHE_LINE)

dnl
dnl OS string
dnl
AC_ARG_ENABLE(os_string, AC_HELP_STRING([--enable-os-string=STR], [Set a customized OS type string]), [os_string="$enableval"])
AC_DEFINE_UNQUOTED(OS_TYPE, "${os_string}", [OS type])

dnl
dnl Tracing
dnl
AC_ARG_ENABLE(trace, AC_HELP_STRING([--enable-trace], [Enable trace facility]),
   [enable_trace="$enableval"
    AC_DEFINE(TRACE_ENABLED, 1, [Trace facility])
   ], [enable_trace="no"])

dnl
dnl Backtraces
dnl
AC_ARG_ENABLE(backtraces, AC_HELP_STRING([--enable-backtraces], [Enable backtraces on error]),
   [enable_backtraces="$enableval"
    AC_DEFINE(BACKTRACES_ENABLED, 1, [Backtracing facility])
   ], [enable_backtraces="no"])

dnl
dnl Dynamic library loading library
dnl
LIBS="$LIBS $libdl"
AC_DEFINE_UNQUOTED(SO_SUFFIX, "${so_suffix}", [Dynamic loading libraries extension])
AC_DEFINE_UNQUOTED(MOD_SUFFIX, "${mod_suffix}", [Dynamic modules extension])

dnl
dnl Libtool options
dnl
LIBTOOL_EXPORT_OPTIONS=
AC_SUBST(LIBTOOL_EXPORT_OPTIONS)

dnl Set the Version
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS(config.h)
AC_SUBST(VERSION)

dnl Initialize Libtool
m4_ifdef([LT_INIT], [LT_INIT([dlopen shared static])], [AC_LIBTOOL_DLOPEN])

dnl Paths
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL

AC_PATH_PROG(cherokeepath, cherokee)

AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_VPRINTF

AC_C_CONST
AC_C_BIGENDIAN
AC_C_INLINE
if test "$ac_cv_c_inline" != no; then
  AC_DEFINE(HAVE_INLINE, 1, [Compile supports inline])
fi

dnl
dnl Check for headers
dnl
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_HEADER_SYS_WAIT

AC_CHECK_HEADERS(endian.h machine/endian.h sys/machine.h sys/endian.h sys/isa_defs.h sys/utsname.h sys/poll.h poll.h )
AC_CHECK_HEADERS(sys/socket.h sys/un.h netinet/in.h arpa/inet.h netinet/tcp.h sys/ioctl.h fcntl.h sys/ofcntl.h sys/time.h)
AC_CHECK_HEADERS(sys/resource.h resource.h unistd.h syslog.h stdint.h inttypes.h error.h pwd.h sys/uio.h)
AC_CHECK_HEADERS(pthread.h netdb.h stdarg.h sys/filio.h sys/varargs.h sys/select.h sys/mman.h sys/uio.h grp.h)
AC_CHECK_HEADERS(sched.h execinfo.h)

AC_SYS_LARGEFILE

AC_SIZE_T
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_STRUCT_ST_RDEV
AC_CHECK_TYPE(ino_t, unsigned)
AC_CHECK_TYPE(loff_t, off_t)
AC_CHECK_TYPE(offset_t, loff_t)
AC_CHECK_TYPE(ssize_t, int)
AC_CHECK_TYPE(wchar_t, unsigned short)

AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(time_t)

AC_CACHE_CHECK([for long long],samba_cv_have_longlong,[
AC_TRY_RUN([#include <stdio.h>
main() { long long x = 1000000; x *= x; exit(((x/1000000) == 1000000)? 0: 1); }],
samba_cv_have_longlong=yes,samba_cv_have_longlong=no,samba_cv_have_longlong=cross)])
if test x"$samba_cv_have_longlong" = x"yes"; then
    AC_DEFINE(HAVE_LONGLONG,1,[Whether the host supports long long'])
fi

AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_FORK

AC_CHECK_FUNCS(gmtime gmtime_r localtime localtime_r getrlimit getdtablesize readdir readdir_r flockfile funlockfile strnstr backtrace random srandom srandomdev)

FW_CHECK_PWD
FW_CHECK_GRP

AC_CHECK_MEMBER(struct tm.tm_gmtoff,
                AC_DEFINE([HAVE_STRUCT_TM_GMTOFF],[1],[gmtoff in struct tm]),,[#include <time.h>])

AH_BOTTOM([
/* Give us an unsigned 32-bit data type. */
#if SIZEOF_UNSIGNED_LONG==4
#define UWORD32 unsigned long
#elif SIZEOF_UNSIGNED_INT==4
#define UWORD32 unsigned int
#else
#error I do not know what to use for a UWORD32.
#endif
])


dnl Find socket()
dnl Likely combinations:
dnl		-lsocket [ -lnsl_s | -lnsl ]
dnl		-linet
AC_CHECK_LIB(ws2_32, main)
AC_CHECK_LIB(advapi32, main)

AC_CHECK_FUNC(socket, :, [
	AC_CHECK_LIB(socket, main)
	AC_CHECK_LIB(net, main)
	AC_CHECK_LIB(nsl_s, main)
	AC_CHECK_LIB(nsl, main)
	AC_CHECK_LIB(inet, socket)
	AC_CHECK_LIB(gen, main)
])

AC_SEARCH_LIBS(accept, socket)
AC_SEARCH_LIBS(bind, socket)
AC_SEARCH_LIBS(listen, socket)
AC_SEARCH_LIBS(setsockopt, socket)

dnl
dnl Check for fd events inspect functions
dnl
AC_CHECK_FUNC(poll, have_poll=yes)
AC_CHECK_FUNC(kqueue, have_kqueue=yes)
AC_CHECK_FUNC(select, have_select=yes)

dnl
dnl Epoll
dnl
AC_CHECK_HEADER(sys/epoll.h, have_epoll_include=yes, have_epoll_include=no)

AC_ARG_ENABLE(epoll, AC_HELP_STRING([--disable-epoll],[Disable epoll() support]),
		    wants_epoll="$enableval", wants_epoll="yes")

have_epoll=no
if test "x$have_epoll_include" = "xyes" && test "x$wants_epoll" = "xyes"; then
 	AC_MSG_CHECKING(for epoll system call)

     AC_RUN_IFELSE([AC_LANG_SOURCE([
		#include <stdint.h>
		#include <sys/param.h>
		#include <sys/types.h>
		#include <sys/syscall.h>
		#include <sys/epoll.h>
		#include <unistd.h>

		int epoll_create (int size) {
	        return (syscall(__NR_epoll_create, size));
		}

		int main (int argc, char **argv) {
	        int epfd;
	        epfd = epoll_create(256);
	    	   exit (epfd == -1 ? 1 : 0);
		}
	])],
	have_epoll=yes,
	have_epoll=no,
	have_epoll=yes)
	AC_MSG_RESULT($have_epoll)
fi

dnl
dnl Solaris 10: Event ports
dnl
AC_CHECK_HEADER(port.h, have_port_include=yes, have_port_include=no)

have_port=no
if test "x$have_port_include" = "xyes"; then
    	AC_MSG_CHECKING(for event ports support)

     AC_RUN_IFELSE([AC_LANG_SOURCE([
		#include <stdint.h>
		#include <unistd.h>
		#include <port.h>

		int main (int argc, char **argv) {
	        int port;
		   port = port_create();
	    	   exit (port < 0 ? 1 : 0);
		}
	])],
	have_port=yes,
	have_port=no,
	have_port=cross)
	AC_MSG_RESULT($have_port)
fi



if test "$have_epoll" = yes; then
	AC_DEFINE(HAVE_EPOLL, 1, [Have epoll])
fi
AM_CONDITIONAL(COMPILE_EPOLL, test x"$have_epoll" = "xyes")

if test "$have_kqueue" = yes; then
	AC_DEFINE(HAVE_KQUEUE, 1, [Have kqueue])
fi
AM_CONDITIONAL(COMPILE_KQUEUE, test x"$have_kqueue" = "xyes")

if test "$have_port" = yes; then
	AC_DEFINE(HAVE_PORT, 1, [Have event ports])
fi
AM_CONDITIONAL(COMPILE_PORT, test x"$have_port" = "xyes")

if test "$have_poll" = yes; then
	AC_DEFINE(HAVE_POLL, 1, [Have poll])
fi
AM_CONDITIONAL(COMPILE_POLL, test x"$have_poll" = "xyes")

if test "$have_select" = yes; then
     AC_DEFINE(HAVE_SELECT, 1, [Have select])
fi
AM_CONDITIONAL(COMPILE_SELECT, test x"$have_select" = "xyes")

dnl
dnl Check for inet_pton.  We have our own, but on Solaris the version in
dnl libresolv is more lenient in ways that Solaris's internal DNS resolution
dnl code requires, so if we use our own *and* link with libresolv (which may
dnl be forced by Perl) DNS resolution fails.
dnl
AC_SEARCH_LIBS(inet_pton, [socket nsl resolv])
AC_CHECK_FUNCS(inet_pton inet_ntop inet_addr)

AC_SEARCH_LIBS(gethostbyname, [socket nsl resolv])
AC_CHECK_FUNCS(gethostbyname gethostbyname_r)

AC_CHECK_HEADER(sys/utsname.h)
AC_CHECK_FUNCS(gethostname uname)

dnl
dnl Check for inet_addr
dnl
AC_SEARCH_LIBS(inet_addr, xnet)
AC_CHECK_FUNCS(inet_addr)

dnl
dnl TCP_CORK
dnl
HYDRA_TCP_CORK
HYDRA_TCP_NOPUSH

dnl
dnl Check for GNU getopt_long()
dnl
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_FUNC(getopt_long, have_getopt_long="yes")
AM_CONDITIONAL(HAVE_GETOPT_LONG, test "x$have_getopt_long" = "xyes")


dnl
dnl Pthread
dnl
with_pthread="yes"
have_pthread="no"
AC_ARG_ENABLE(pthread, AC_HELP_STRING([--disable-pthread],[Disable threading support]),
		    with_pthread="$enableval", with_pthread="yes")

AM_CONDITIONAL(USE_PTHREAD, test "x$with_pthread" = "xyes")
if test "x$with_pthread" = "xyes"
then
	dnl
     dnl Basic checkings (c&p'ed from squid)
	dnl
	AC_MSG_CHECKING([for special a pthread case])

	oldcflags="$CFLAGS"
     CFLAGS="$CFLAGS -D_REENTRANT"
	PTHREAD_CFLAGS="-D_REENTRANT"

	case "$host" in
    	   i386-unknown-freebsd*)
           if test "$GCC" = "yes" ; then
              if test -z "$PRESET_LDFLAGS"; then
                PTHREAD_LIBS="-pthread"
			 have_pthread="yes"
              fi
           fi
		 AC_MSG_RESULT([-pthread])
           ;;
       *-solaris2.*)
           if test "$GCC" = "yes" ; then
              CFLAGS="$CFLAGS -pthreads"
              PTHREAD_CFLAGS="-pthreads"
		    AC_MSG_RESULT([-pthreads])
           else
              CFLAGS="$CFLAGS -mt"
              PTHREAD_CFLAGS="-mt"
		    AC_MSG_RESULT([-mt])
           fi
	      have_pthread="yes"
           ;;
	  *)
		 AC_MSG_RESULT([no])
 		 ;;
     esac

	if test "$have_pthread" != "yes"; then
        AC_CHECK_LIB(pthread, main, have_pthread=yes, have_pthread=no)
	   if test "$have_pthread" = "yes"; then
	   	 PTHREAD_LIBS="-lpthread"
	   fi
     fi

	dnl
	dnl More detailed checks on the pthread support
	dnl
	AC_MSG_CHECKING([for pthread_rwlock_t support])

	have_pthread_rwlock_t=yes
	AC_TRY_COMPILE([#include <pthread.h>],
			[pthread_rwlock_t rwlock; pthread_rwlock_init( &rwlock, NULL);],
			compiled=yes, compiled=no)

	if test "$compiled" = "yes"; then
	   	AC_MSG_RESULT([ok])
	else
	     dnl Didn't find rwlock_t.
	     dnl Try defining _XOPEN_SOURCE=500
	     dnl
		oldcflags2="$CFLAGS"
		CFLAGS="$CFLAGS -D_XOPEN_SOURCE=500"

		AC_TRY_COMPILE([#include <pthread.h>],
			    [pthread_rwlock_t rwlock; pthread_rwlock_init( &rwlock, NULL);],
			    compiled=yes, compiled=no)

		if test "$compiled" = "yes"; then
		    AC_MSG_RESULT([-D_XOPEN_SOURCE=500])
		    PTHREAD_CFLAGS="-D_XOPEN_SOURCE=500"
		else
		    have_pthread_rwlock_t=no
		    AC_MSG_RESULT([no])
		fi

		CFLAGS="$oldcflags2"
	fi

	if test "$have_pthread_rwlock_t" = "yes"; then
		AC_DEFINE(HAVE_PTHREAD_RWLOCK_T, 1, [Define if your pthread library includes pthread_rwlock_t])
	else
		AC_MSG_ERROR([pthread_rwlock_t support missing])
	fi

	AC_CHECK_FUNC(pthread_attr_setschedpolicy, have_pthread_attr_setschedpolicy=yes)
	if test x"$have_pthread_attr_setschedpolicy" = "xyes"; then
	     AC_DEFINE(HAVE_PTHREAD_SETSCHEDPOLICY, 1, [Pthread support pthread_attr_setschedpolicy])
	fi

	save_LIBS="$LIBS"
	LIBS="$LIBS $PTHREAD_LIBS"

	AC_CHECK_FUNCS(pthread_mutexattr_settype pthread_mutexattr_setkind_np)

	dnl
	dnl Yield
	dnl
	AC_CHECK_FUNCS(sched_yield, , [
	   AC_CHECK_LIB(rt, sched_yield, [
	       AC_DEFINE(HAVE_SCHED_YIELD)
		  PTHREAD_LIBS="$PTHREAD_LIBS -lrt"
	   ],[
	       AC_CHECK_LIB(posix4, sched_yield, [
		  AC_DEFINE(HAVE_SCHED_YIELD)
      	  PTHREAD_LIBS="$PTHREAD_LIBS -lposix4"
   	   ])])
     ])

	LIBS="$save_LIBS"
	CFLAGS="$oldcflags"
fi

if test "$have_pthread" = "yes"; then
	AC_DEFINE(HAVE_PTHREAD, 1, [Have pthread support])
	AC_SUBST(PTHREAD_CFLAGS)
	AC_SUBST(PTHREAD_LIBS)
fi


dnl
dnl Is setenv() threadsafe?
dnl
if test "$setenv_is_threadsafe" = "yes"; then
AC_DEFINE(SETENV_IS_THREADSAFE, 1, [setenv function is thread safe])
fi

dnl
dnl Check for vsyslog
dnl
AC_CHECK_FUNCS(syslog vsyslog strsep strcasestr memmove strerror bcopy strlcat)

dnl
dnl Check for Glib (usually Linux)
dnl
AC_PREPROC_IFELSE([AC_LANG_SOURCE([
#include <features.h>
#ifndef __GLIBC__
    chokeme
#endif
])], [
    AC_DEFINE([HAVE_GLIBC], [1], [Define to 1 if you have glibc.])
    AC_DEFINE([_GNU_SOURCE], [1], [Define to 1 if you have glibc.])
])

dnl
dnl Does this platform require array notation to assign to a va_list?
dnl If cross-compiling, we assume va_list is "normal".  If this breaks
dnl you, set ac_cv_valistisarray=true and maybe define HAVE_VA_LIST_AS_ARRAY
dnl also just to be sure.
dnl
AC_MSG_CHECKING(whether va_list assignments need array notation)
AC_CACHE_VAL(ac_cv_valistisarray,
        [AC_TRY_RUN([#include <stdlib.h>
		     #include <stdarg.h>
		     void foo(int i, ...) {
			va_list ap1, ap2;
			va_start(ap1, i);
			ap2 = ap1;
			if (va_arg(ap2, int) != 123 || va_arg(ap1, int) != 123) { exit(1); }
			va_end(ap1); va_end(ap2);
		     }
		     int main()
		     { foo(0, 123); return(0); }],
		    [ac_cv_valistisarray=false],
		    [ac_cv_valistisarray=true],
		    [ac_cv_valistisarray=false])])

if test "$ac_cv_valistisarray" = true ; then
	AC_DEFINE(HAVE_VA_LIST_AS_ARRAY,,[va_list works copying an array])
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi


dnl
dnl Check if the global variable `timezone' exists. If so, define
dnl HAVE_INT_TIMEZONE.
dnl
AC_STRUCT_TIMEZONE

AC_MSG_CHECKING(for global timezone)
AC_TRY_LINK([#include <time.h>],
[int res; res = timezone / 60;],
[AC_MSG_RESULT(yes)
AC_DEFINE(HAVE_INT_TIMEZONE,, [Set to 1 if you have the global variable timezone])],
AC_MSG_RESULT(no))


dnl
dnl Check for RTDL constants (from Haskell code)
dnl
dnl ** check for libdl & RTLD_NEXT
dnl ** sometimes RTLD_NEXT is hidden in #ifdefs we really don't wan to set
AC_MSG_CHECKING(for RTLD_NEXT from dlfcn.h)
AC_EGREP_CPP(yes,
[
 #include <dlfcn.h>
 #ifdef RTLD_NEXT
        yes
 #endif
], [
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_RTLDNEXT,,[Have RTDL_NEXT])
  HaveRtldNext=YES
], [
  AC_MSG_RESULT(no)
  HaveRtldNext=NO
  ])
AC_SUBST(HaveRtldNext)

dnl ** RTLD_LOCAL isn't available on cygwin or openbsd
AC_MSG_CHECKING(for RTLD_LOCAL from dlfcn.h)
AC_EGREP_CPP(yes,
[
 #include <dlfcn.h>
 #ifdef RTLD_LOCAL
        yes
 #endif
], [
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_RTLDLOCAL,,[Have RTDL_LOCAL])
  HaveRtldLocal=YES
], [
  AC_MSG_RESULT(no)
  HaveRtldLocal=NO
  ])
AC_SUBST(HaveRtldLocal)

dnl ** RTLD_GLOBAL isn't available on openbsd
AC_MSG_CHECKING(for RTLD_GLOBAL from dlfcn.h)
AC_EGREP_CPP(yes,
[
 #include <dlfcn.h>
 #ifdef RTLD_GLOBAL
        yes
 #endif
], [
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_RTLDGLOBAL,,[Have RTDL_GLOBAL])
  HaveRtldGlobal=YES
], [
  AC_MSG_RESULT(no)
  HaveRtldGlobal=NO
  ])
AC_SUBST(HaveRtldGlobal)

dnl ** RTLD_NOW isn't available on openbsd
AC_MSG_CHECKING(for RTLD_NOW from dlfcn.h)
AC_EGREP_CPP(yes,
[
 #include <dlfcn.h>
 #ifdef RTLD_NOW
        yes
 #endif
], [
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_RTLDNOW,,[Have RTDL_NOW])
  HaveRtldNow=YES
], [
  AC_MSG_RESULT(no)
  HaveRtldNow=NO
  ])
AC_SUBST(HaveRtldNow)


dnl
dnl Check of off64_t
dnl
AC_CACHE_CHECK([for off64_t],samba_cv_HAVE_OFF64_T,[
AC_TRY_RUN([
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <stdio.h>
#include <sys/stat.h>
main() { struct stat64 st; off64_t s; if (sizeof(off_t) == sizeof(off64_t)) exit(1); exit((lstat64("/dev/null", &st)==0)?0:1); }],
samba_cv_HAVE_OFF64_T=yes,samba_cv_HAVE_OFF64_T=no,samba_cv_HAVE_OFF64_T=cross)])
if test x"$samba_cv_HAVE_OFF64_T" = x"yes"; then
    AC_DEFINE(HAVE_OFF64_T,1,[Whether off64_t is available])
fi


dnl
dnl SYSV Semaphores
dnl
AC_CHECK_HEADER(sys/sem.h, sysv_sem=yes, sysv_sem=no)

if test "x$sysv_sem" = "xyes"; then
   AC_CHECK_FUNC(semget, , sysv_sem=no)
   AC_CHECK_FUNC(semop, , sysv_sem=no)
   AC_CHECK_FUNC(semctl, , sysv_sem=no)
fi

if test "x$sysv_sem" = xyes; then
   AC_DEFINE(HAVE_SYSV_SEMAPHORES, 1, [Define to 1 to use SYSV semaphores])
fi
AM_CONDITIONAL(USE_SYSV_SEMAPHORES, test "x$sysv_sem" = xyes)

dnl union semun
dnl
AC_MSG_CHECKING([for union semun])
AC_CACHE_VAL(ac_cv_struct_semun, [
   AC_TRY_COMPILE(
	[
	   #include <sys/types.h>
	   #include <sys/ipc.h>
	   #include <sys/sem.h>;
     ],[
	   union semun semdat;
	],
	   ac_cv_struct_semun=yes,
	   ac_cv_struct_semun=no
     )
])
AC_MSG_RESULT($ac_cv_struct_semun)
if test "$ac_cv_struct_semun" = "yes"; then
	AC_DEFINE(HAVE_SEMUN, 1, [Define if sys/sem.h defines struct semun])
fi


# From etr_socket_nsl.m4
ETR_SOCKET_NSL

# From sendfile_samba.m4
SENDFILE_CHECK

# readdir_r()
LIBWWW_READDIR_R_TYPE


dnl
dnl IPv6 Support
dnl I got this mostly from Apache's tests
dnl

AC_ARG_ENABLE(ipv6,
  AC_HELP_STRING([--disable-ipv6], [Disable IPv6 support]),
  [ if test "$enableval" = "no"; then
       disabled_ipv6=1
    fi ],
  [ disabled_ipv6=0 ] )

AC_SEARCH_LIBS(getaddrinfo, socket inet6)
AC_SEARCH_LIBS(getnameinfo, socket inet6)
AC_SEARCH_LIBS(gai_strerror, socket inet6)
AC_CHECK_FUNC(gai_strerror)

APR_CHECK_WORKING_GETADDRINFO
APR_CHECK_WORKING_GETNAMEINFO

AC_ACME_SOCKADDR_UN
AC_ACME_SOCKADDR_IN6

AC_MSG_CHECKING(if the system supports IPv6)
have_ipv6="no"

if test "$disabled_ipv6" = 1; then
    AC_MSG_RESULT([no -- disabled by user])
else
    if test "x$ac_cv_acme_sockaddr_in6" = "xyes"; then
        if test "x$ac_cv_working_getaddrinfo" = "xyes"; then
            if test "x$ac_cv_working_getnameinfo" = "xyes"; then
                have_ipv6="yes"
                AC_MSG_RESULT([yes])
 		      AC_DEFINE(HAVE_IPV6, 1, [Define if you have IPv6 support.])
            else
                AC_MSG_RESULT([no -- no getnameinfo])
            fi
        else
            AC_MSG_RESULT([no -- no working getaddrinfo])
        fi
    else
        AC_MSG_RESULT([no -- no sockaddr_in6])
    fi
fi

dnl
dnl Test whether SO_RCVTIMEO is broken
dnl
AC_CACHE_CHECK([whether setsockopt(SO_RCVTIMEO) is broken],
  ac_cv_so_rcvtimeo_broken, [dnl
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

int main(void) {
        int fd;
        int ret;
        struct timeval new_tv;

        /* Open the socket (INET/TCP).*/
        fd = socket(AF_INET, SOCK_STREAM, 0);

        /* set the timeout for the incoming queue */
        /* 1 second for example */
        new_tv.tv_sec = 1;
        new_tv.tv_usec = 0;

        ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &new_tv, sizeof(new_tv));
        return ret;
}
]])],[ac_cv_so_rcvtimeo_broken=no],[ac_cv_so_rcvtimeo_broken=yes],[ac_cv_so_rcvtimeo_broken=cross])])
if test x"$ac_cv_so_rcvtimeo_broken" = x"yes"; then
  AC_DEFINE(HAVE_BROKEN_SO_RCVTIMEO, 1, [Define if setsockopt(SO_RCVTIMEO) is broken])
fi


dnl
dnl Look for socklen_t
dnl
AC_MSG_CHECKING([for socklen_t])
AC_TRY_COMPILE(
 [#include <sys/types.h>
  #include <sys/socket.h>],
 [socklen_t len = 42; return 0;],
 ac_cv_type_socklen_t=yes,
 ac_cv_type_socklen_t=no
)

if test $ac_cv_type_socklen_t != yes; then
  AC_DEFINE(socklen_t, int, [Substitute for socklen_t])
  AC_MSG_RESULT(int)
else
  AC_MSG_RESULT(yes)
fi

dnl
dnl Check of __func__ and co..
dnl
AC_MSG_CHECKING([whether our compiler supports __func__])
AC_TRY_COMPILE([],
 [const char *cp = __func__;],
 AC_MSG_RESULT([yes]),
 AC_MSG_RESULT([no])
 AC_MSG_CHECKING([whether our compiler supports __FUNCTION__])
 AC_TRY_COMPILE([],
   [const char *cp = __FUNCTION__;],
   AC_MSG_RESULT([yes])
   AC_DEFINE(__func__, __FUNCTION__,
         [Define to appropriate substitue if compiler doesnt have __func__]),
   AC_MSG_RESULT([no])
   AC_DEFINE(__func__, __FILE__,
         [Define to appropriate substitue if compiler doesnt have __func__])))


dnl
dnl Temporal directory
dnl
AC_ARG_ENABLE(tmpdir, AC_HELP_STRING([--enable-tmpdir=DIR],[default directory for temporary files]))

AC_MSG_CHECKING(directory to use for temporary files)
if test -n "$enable_tmpdir"; then
        tmpdir="$enable_tmpdir"
elif test -n "$TMPDIR"; then
        tmpdir="$TMPDIR"
elif test -n "$TMP"; then
        tmpdir="$TMP"
elif test -n "$TEMP"; then
        tmpdir="$TEMP"
elif test -d "c:/"; then
        tmpdir="c:/"
else
        tmpdir="/tmp"
fi

dnl No temp dir
if ! test -d $tmpdir ; then
   AC_MSG_ERROR($tmpdir does not exist)
fi

dnl Handle MacOS X messy tmp dirs
case "$tmpdir" in
  /var/folders*)
     tmpdir='/tmp'
	;;
esac

AC_MSG_RESULT($tmpdir)
AC_DEFINE_UNQUOTED(TMPDIR, "$tmpdir", [Temporal directory])


dnl
dnl Check for pcre library
dnl
have_pcre="built-in"

AC_ARG_ENABLE(internal_pcre,
		    AC_HELP_STRING([--enable-internal-pcre],[Enable internal PCRE]),
		    use_internal_pcre="$enableval", use_internal_pcre="no")

if test "x$use_internal_pcre" != "xyes"; then
  AC_CHECK_LIB(pcre, pcre_compile, have_pcre_lib=yes, have_pcre_lib=no)
  AC_CHECK_HEADER(pcre.h, have_pcre_include=yes, have_pcre_include=no)
  if test "$have_pcre_lib $have_pcre_include" = "yes yes"; then
     have_pcre="yes"
  fi
fi

AM_CONDITIONAL(USE_INTERNAL_PCRE, test "x$have_pcre" = "xbuilt-in")

dnl
dnl PAM
dnl
AC_ARG_ENABLE(pam, AC_HELP_STRING([--disable-pam],[Disable PAM support]), use_pam="$enableval", use_pam="yes")
if test "x$use_pam" = "xyes"; then
     AC_CHECK_LIB(pam, pam_start, have_pam_lib=yes, have_pam_lib=no)
     AC_CHECK_LIB(pam, _pam_dispatch, have_pam_dispatch=yes, have_pam_dispatch=no)
     AC_CHECK_HEADER(security/pam_modules.h, have_pam_include=yes, have_pam_include=no, [[
#ifdef HAVE_SECURITY_PAM_MODULES_H
# include <security/pam_modules.h>
#endif
	]])
     AC_CHECK_HEADERS(security/_pam_macros.h security/pam_appl.h)
fi

if test "$use_pam $have_pam_lib $have_pam_include" = "yes yes yes"; then
   have_pam="yes"
else
   have_pam="no"
fi
AM_CONDITIONAL(HAVE_PAM, test "x$have_pam" = "xyes")

if test "$have_pam_dispatch" = "yes"; then
	AC_DEFINE(HAVE_PAM_DISPATCH, 1, [Have _pam_dispatch function])
fi

dnl
dnl crypt()
dnl
AC_CHECK_HEADERS(crypt.h)

AC_CHECK_FUNC(crypt, system_crypt="yes", system_crypt="no")
AC_CHECK_LIB(crypt, crypt, have_crypt_lib="yes", have_crypt_lib="no")
if test "x$have_crypt_lib" = "xyes"; then
	CRYPT_LIBS="-lcrypt"
	AC_SUBST(CRYPT_LIBS)
fi

use_crypt="no"
if test "$system_crypt $have_crypt_lib" != "no no"; then
   use_crypt="yes"
   AC_DEFINE(HAVE_CRYPT, 1, [Have crypt function])
fi

dnl
dnl crypt_r()
dnl
AC_CHECK_FUNC(crypt_r, system_crypt_r="yes", system_crypt_r="no")
AC_CHECK_LIB(crypt, crypt_r, have_crypt_r_lib="yes", have_crypt_r_lib="no")

use_crypt_r="no"
if test "$system_crypt_r $have_crypt_r_lib" != "no no"; then
   use_crypt_r="yes"
   AC_DEFINE(HAVE_CRYPT_R, 1, [Have crypt_r function])
fi

dnl
dnl crypt_r and pthread
dnl
if test "$use_crypt_r" = "yes"; then
   dnl
   dnl crypt_r style
   dnl
   AC_MSG_CHECKING([crypt_r style])
   crypt_r_style=none

   # pthread.h is needed on AIX
   AC_TRY_COMPILE([#include <pthread.h>
                   #include <crypt.h>],
   			   [CRYPTD buffer;],
			   crypt_r_style=cryptd)

   if test "$crypt_r_style" = "none"; then
      AC_TRY_COMPILE([#include <crypt.h>],
				 [struct crypt_data buffer;],
				 crypt_r_style=struct_crypt_data)
   fi

   if test "$crypt_r_style" = "none"; then
      AC_TRY_COMPILE([#define _GNU_SOURCE
	 			  #include <crypt.h>],
				 [struct crypt_data buffer;],
				 [crypt_r_style=struct_crypt_data
			       CRYPT_CFLAGS="-D_GNU_SOURCE"
				  AC_SUBST(CRYPT_CFLAGS)])
   fi

   AC_MSG_RESULT([$crypt_r_style])

   if test "$crypt_r_style" = "cryptd"; then
     AC_DEFINE(CRYPT_R_CRYPTD, 1, [Define if crypt_r has uses CRYPTD])
   fi

   if test "$crypt_r_style" = "struct_crypt_data"; then
     AC_DEFINE(CRYPT_R_STRUCT_CRYPT_DATA, 1, [Define if crypt_r uses struct crypt_data])
   fi

   if test "$crypt_r_style" = "none"; then
     AC_MSG_ERROR([Unable to detect data struct is used by crypt_r])
   fi
fi

AM_CONDITIONAL(USE_CRYPT, test "x$use_crypt x$use_crypt_r" != "xno xno")


dnl
dnl OpenSSL
dnl
AC_ARG_WITH(libssl,
    AC_HELP_STRING([--with-libssl@<:@=DIR@:>@],[Include libssl/OpenSSL support (default yes)]),
    [WITH_OPENSSL=$withval],[WITH_OPENSSL=yes])

have_openssl=no

if test "$WITH_OPENSSL" != "no"; then
    if test "$WITH_OPENSSL" != "yes"; then
        LIBSSL_CFLAGS="-I$WITH_OPENSSL/include"
        LIBSSL_LIBS="-L$WITH_OPENSSL/lib"
    fi

    AC_CHECK_LIB(crypto, BN_init)
    AC_CHECK_HEADERS([openssl/engine.h])
    AC_CHECK_LIB(ssl, SSL_accept, [have_openssl=yes], [have_openssl=no], $LIBSSL_LIBS)

    if test "$have_openssl" = "yes"; then
        LIBSSL_LIBS="$LIBSSL_LIBS -lssl"

	   AC_SUBST(LIBSSL_LIBS)
	   AC_SUBST(LIBSSL_CFLAGS)

	   AC_DEFINE(HAVE_OPENSSL, 1, [Have OpenSSL library])
	   AC_PATH_PROG([OPENSSL_BIN], [openssl])
	   AC_SUBST(OPENSSL_BIN)
    fi
fi
AC_MSG_CHECKING(for libssl)
AC_MSG_RESULT([$have_openssl])

AM_CONDITIONAL(USE_OPENSSL, test "x$have_openssl" = "xyes")


dnl
dnl Native Language Support
dnl
AM_PO_SUBDIRS

if test "x$USE_NLS" = "xyes"; then
   AC_CHECK_PROG(HAVE_MSGFMT, msgfmt,yes,no)

   if test "x$HAVE_MSGFMT" = "xno"; then
      AC_MSG_ERROR([msgfmt not found. You need to install the 'gettext' package, or pass --enable-nls=no to configure.])
   fi
fi

AM_CONDITIONAL(NLS_ENABLED, test "x$USE_NLS" = "xyes")
AC_CONFIG_FILES([po/admin/Makefile.in])

dnl
dnl LDAP
dnl
AC_ARG_WITH([ldap],
        AC_HELP_STRING([--with-ldap=@<:@ARG@:>@],
            [use OpenLDAP development library @<:@default=yes@:>@, optionally specify path to dev libs]
        ),
        [
        if test "$withval" = "no"; then
            want_ldap="no"
        elif test "$withval" = "yes"; then
            want_ldap="yes"
        else
            want_ldap="yes"
            LDAP_CONFIG="$withval"
        fi
        ],
        [want_ldap="yes"]
)

if test "$want_ldap" = "yes"; then
       AC_CHECK_LIB(ldap, ldap_init, have_ldap_lib=yes, have_ldap_lib=no)
       AC_CHECK_HEADER(ldap.h, have_ldap_include=yes, have_ldap_include=no)

	  AC_CHECK_LIB(ldap, ldap_start_tls_s, [
		AC_DEFINE(LDAP_HAVE_START_TLS_S,, Define if you have ldap_start_tls_s)
  	  ])

       if test "$have_ldap_lib $have_ldap_include" = "yes yes"; then
          have_ldap="yes"
       else
          have_ldap="no"
       fi
else
       have_ldap="no"
fi

AM_CONDITIONAL(HAVE_LDAP, test "x$have_ldap" = "xyes")


dnl
dnl Check for MySQL: Execution of a string containing multiple
dnl statements was introduced in the MySQL release 5.0.0
dnl
AX_LIB_MYSQL(5.0.0)


dnl
dnl GeoIP library
dnl
AC_ARG_WITH([geoip],
        AC_HELP_STRING([--with-geoip=@<:@ARG@:>@],
            [use the GeoIP library @<:@default=yes@:>@, optionally specify path to dev libs]
        ),
        [
        if test "$withval" = "no"; then
            want_geoip="no"
        elif test "$withval" = "yes"; then
            want_geoip="yes"
        else
            want_geoip="yes"
		  CPPFLAGS="${CPPFLAGS} -I${withval}/include"
		  LDFLAGS="-L${withval}/lib ${LDFLAGS}"
        fi
        ],
        [want_geoip="yes"]
)

if test "x$want_geoip" = "xyes"; then
   AC_CHECK_LIB(GeoIP, GeoIP_new, have_geoip_lib=yes, have_geoip_lib=no)
   AC_CHECK_HEADERS(GeoIP.h, have_geoip_include=yes, have_geoip_include=no)
fi

have_geoip="no"
if test "$have_geoip_lib $have_geoip_include" = "yes yes"; then
   have_geoip="yes"
fi

AM_CONDITIONAL(HAVE_GEOIP, test "x$have_geoip" = "xyes")


dnl
dnl FFMpeg library
dnl
AC_ARG_WITH([ffmpeg],
        AC_HELP_STRING([--with-ffmpeg=@<:@ARG@:>@],
            [use the FFMpeg library @<:@default=yes@:>@, optionally specify path to dev libs]
        ),
        [
        if test "$withval" = "no"; then
            want_ffmpeg="no"
        elif test "$withval" = "yes"; then
            want_ffmpeg="yes"
        else
            want_ffmpeg="yes"
            FFMPEG_CONFIG="$withval"
        fi
        ],
        [want_ffmpeg="yes"]
)

if test "x$want_ffmpeg" = "xyes"; then
   AC_CHECK_LIB([avformat], [main], [
	 FFMPEG_LIBS="-lavformat -lavcodec -lavutil"
   	 have_ffmpeg_lib=yes
   ],[have_ffmpeg_lib=no])

   AC_CHECK_LIB([bz2], [BZ2_bzdopen], [
	 FFMPEG_LIBS="$FFMPEG_LIBS -lbz2"
   ])

   AC_CHECK_LIB([m], [cos], [
	 FFMPEG_LIBS="$FFMPEG_LIBS -lm"
   ])

   AC_CHECK_HEADERS([libavformat/avformat.h], [
      have_ffmpeg_include=yes
   ],[have_ffmpeg_include=no])
fi

have_ffmpeg="no"
if test "$have_ffmpeg_lib $have_ffmpeg_include" = "yes yes"; then
   have_ffmpeg="yes"
   AC_SUBST(FFMPEG_LIBS)
   AC_SUBST(FFMPEG_CFLAGS)
   AC_DEFINE([USE_FFMPEG], 1, [Whether to use FFMpeg])
fi

AM_CONDITIONAL(HAVE_FFMPEG, test "x$have_ffmpeg" = "xyes")


dnl
dnl WWW root directory, and user/group
dnl
AC_ARG_WITH(wwwroot, AC_HELP_STRING([--with-wwwroot=DIR], [Set the WWW root directory]),
		  WWW_ROOT="$withval", [
   dnl MacOS X
   if test -d "/Library/WebServer/Documents"; then
	   WWW_ROOT="/Library/WebServer/Documents"
   dnl Some Linux
   elif test -d "/srv/www/htdocs"; then
	   WWW_ROOT="/srv/www/htdocs"
   dnl Default
   else
	   WWW_ROOT="$localstatedir/www"
   fi
])
AC_SUBST(WWW_ROOT)

AC_ARG_WITH(cgiroot, AC_HELP_STRING([--with-cgiroot=DIR], [Set the CGI directory]),
		  CGI_ROOT="$withval", [
   dnl MacOS X
   if test -d "/Library/WebServer/CGI-Executables"; then
	   CGI_ROOT="/Library/WebServer/CGI-Executables"
   dnl Some Linux
   elif test -d "/srv/www/cgi-bin"; then
	   CGI_ROOT="/srv/www/cgi-bin"
   dnl Default
   else
	   CGI_ROOT="$prefix/lib/cgi-bin"
   fi
])
AC_SUBST(CGI_ROOT)

AC_ARG_WITH(wwwuser, AC_HELP_STRING([--with-wwwuser=USER], [Set default execution user]),
		  WWW_USER="$withval", WWW_USER="")
AC_SUBST(WWW_USER)

AC_ARG_WITH(wwwgroup, AC_HELP_STRING([--with-wwwgroup=GROUP], [Set default execution group]),
		  WWW_GROUP="$withval", WWW_GROUP="")
AC_SUBST(WWW_GROUP)


dnl
dnl PHP fcgi
dnl
AC_ARG_WITH(php, AC_HELP_STRING([--with-php=/path/to/php], [Path to php-fpm or php-cgi]),
	       PHPCGI="$withval", PHPCGI="")

dnl If no php, try to find it
if test "x$PHPCGI" = "x"; then
  for bin in php-fpm php-cgi php; do
    for p in /usr/lib/cgi-bin /usr/sbin /usr/bin /usr/local/sbin /usr/local/bin /opt/local/sbin /opt/local/bin /usr/gnu/sbin /usr/gnu/bin; do
	 if test -x "$p/$bin"; then
        PHPCGI="$p/$bin"
        break
	 fi
    done
    if test "x$PHPCGI" != x; then
	 break
    fi
  done
fi

dnl Try in the $PATH
if test "x$PHPCGI" = "x"; then
   AC_PATH_PROGS(PHPCGI, [php-fpm php-cgi])
fi

dnl Last, desperate option
if test "x$PHPCGI" = "x"; then
   php-cgi -v 1>/dev/null 2>&1
   if test "$?" != "0"; then
      PHPCGI="php-cgi"
   fi
fi

AC_SUBST(PHPCGI)

dnl Ensures it support FastCGI
AC_MSG_CHECKING([$PHPCGI supports FastCGI])
if test "x$PHPCGI" = "x"; then
   AC_MSG_RESULT([no])
else
   $PHPCGI -v -d session.save_path=/tmp | grep 'fcgi' >/dev/null 2>/dev/null
   if test $? != 0; then
      PHPCGI=""
      AC_MSG_RESULT([no, use --with-php])
   else
      AC_MSG_RESULT([yes])
   fi
fi

dnl
dnl Python
dnl
wants_admin="yes"
have_python="yes"

AC_ARG_WITH([python], AC_HELP_STRING([--with-python=/path/to/python],[Path to Python interpreter]),
		  PYTHON="$withval",
[
   AC_PATH_PROGS(PYTHON, python python2 python2.7 python2.6 python2.5 python2.4)
])

if test "x$PYTHON" != "x"; then
   AC_MSG_CHECKING([Python 2.x interpreter])
   v=`$PYTHON -c 'import sys; print (sys.version_info[[0]])'`
   if test "x$v" != "x2"; then
     have_python="no"
     wants_admin="no"
	AC_MSG_RESULT([no])
	AC_MSG_WARN([
*** Cherokee-admin requires Python 2.x. Please, use the --with-python
*** parameter to specify a Python 2.4, 2.5, 2.6 or 2.7 interpreter.
*** BEWARE: Cherokee-admin support has been disabled.])
   else
     AC_MSG_RESULT([yes])
   fi
else
    have_python="no"
    wants_admin="no"
    AC_MSG_WARN([
*** Python must be installed on your system but python interpreter couldn't
*** be found in path. Please check that python is in path, or install it.
*** BEWARE: Cherokee-admin support has been disabled.])
fi

dnl
dnl Cherokee admin
dnl
if test "x$wants_admin" = "xyes"; then
   AC_ARG_ENABLE(admin,
	      AC_HELP_STRING([--disable-admin],[Disable Cherokee-admin installation]),
					 wants_admin="$enableval", wants_admin="yes")
fi
AM_CONDITIONAL(INSTALL_ADMIN, test "x$wants_admin" != "xno")

dnl
dnl Clean files generated from *.pre
dnl
pres="constants.h cherokee.conf.sample performance.conf.sample http-cherokee.xml admin/configured.py contrib/cherokee"

AC_MSG_NOTICE([Deleting .pre files])
rm -f $pres

dnl
dnl Options
dnl
use_static_module=""
AC_ARG_ENABLE(static-module,
	   AC_HELP_STRING([--enable-static-module=MODULE][]),
	   [use_static_module="$use_static_module $enableval "],[])

modules="error_redir error_nn server_info file dirlist cgi fcgi scgi uwsgi proxy redir common ssi secdownload empty_gif drop admin custom_error dbslayer streaming gzip deflate ncsa combined custom pam ldap mysql htpasswd plain htdigest authlist round_robin ip_hash failover directory extensions request header exists fullpath method from bind tls geoip url_arg v_or wildcard rehost target_ip evhost post_track post_report libssl render_rrd rrd not and or"

# Remove modules that will not be compiles
#
if test "x$have_pam" != "xyes"; then
	modules=`echo $modules | sed s/pam//`
fi
if test "x$have_ldap" != "xyes"; then
	modules=`echo $modules | sed s/ldap//`
fi
if test "x$have_mysql" != "xyes"; then
	modules=`echo $modules | sed s/mysql//`
	modules=`echo $modules | sed s/dbslayer//`
fi
if test "x$have_geoip" != "xyes"; then
	modules=`echo $modules | sed s/geoip//`
fi
if test "$use_crypt" != "yes"; then
	modules=`echo $modules | sed s/htpasswd//`
fi
if test "$have_openssl" != "yes"; then
	modules=`echo $modules | sed s/libssl//`
fi
if test "x$have_ffmpeg" != "xyes"; then
	modules=`echo $modules | sed s/streaming//`
fi

add_calls=""
init_calls=""
headers=""
ext_defs=""

for mod in $modules; do
	AC_MSG_CHECKING([module "$mod"])

	if echo $use_static_module | grep -w $mod >/dev/null || echo $use_static_module | grep all >/dev/null; then
		 ext_defs="$ext_defs \n extern cherokee_plugin_info_t cherokee_${mod}_info;"

		 headers="$headers \n void PLUGIN_INIT_NAME($mod) (cherokee_plugin_loader_t *loader);"
		 add_calls="$add_calls \n add_static_entry (loader, \"$mod\", PLUGIN_INFO_PTR($mod));"
		 init_calls="$init_calls \n PLUGIN_INIT_NAME($mod)(loader);"

	      AC_MSG_RESULT([static])
	else
	      AC_MSG_RESULT([dynamic])
	fi
done

AC_MSG_CHECKING([loader.autoconf files])
mkdir cherokee 2>/dev/null
printf "$ext_defs\n"                                 > cherokee/loader.autoconf.h
printf "$headers \n\n $add_calls \n\n $init_calls\n" > cherokee/loader.autoconf.inc
AC_MSG_RESULT([done])

dnl
dnl Static/Dynamic modules
dnl
conf_h="cherokee/loader.autoconf.h"
AM_CONDITIONAL(STATIC_HANDLER_SERVER_INFO,    grep server_info    $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_FILE,           grep file           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_ADMIN,          grep admin          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_DIRLIST,        grep dirlist        $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_CGI,            grep cgi            $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_FCGI,           grep fcgi           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_SCGI,           grep scgi           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_UWSGI,          grep uwsgi          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_REDIR,          grep redir          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_ERROR_REDIR,    grep error_redir    $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_ERROR_NN,       grep error_nn       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_COMMON,         grep common         $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_CGI,            grep cgi            $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_PROXY,          grep proxy          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_SSI,            grep ssi            $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_SECDOWNLOAD,    grep secdownload    $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_EMPTY_GIF,      grep empty_gif      $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_DROP,           grep drop           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_CUSTOM_ERROR,   grep custom_error   $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_DBSLAYER,       grep dbslayer       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_STREAMING,      grep streaming      $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_HANDLER_RENDER_RRD,     grep render_rrd     $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_ENCODER_GZIP,           grep gzip           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_ENCODER_DEFLATE,        grep deflate        $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_LOGGER_NCSA,            grep ncsa           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_LOGGER_COMBINED,        grep combined       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_LOGGER_CUSTOM,          grep custom         $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VALIDATOR_PAM,          grep pam            $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VALIDATOR_LDAP,         grep ldap           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VALIDATOR_MYSQL,        grep mysql          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VALIDATOR_HTPASSWD,     grep htpasswd       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VALIDATOR_PLAIN,        grep plain          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VALIDATOR_HTDIGEST,     grep htdigest       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VALIDATOR_AUTHLIST,     grep authlist       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_BALANCER_ROUND_ROBIN,   grep round_robin    $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_BALANCER_IP_HASH,       grep ip_hash        $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_BALANCER_FAILOVER,      grep failover       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_CRYPTOR_LIBSSL,         grep libssl         $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_DIRECTORY,         grep directory      $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_EXTENSIONS,        grep extensions     $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_REQUEST,           grep request        $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_HEADER,            grep header         $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_EXISTS,        	 grep exists         $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_FULL_PATH,         grep fullpath       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_BIND,        	 grep bind           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_TLS,        	 	 grep tls            $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_METHOD,        	 grep method         $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_FROM,        	 grep from           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_GEOIP,             grep geoip          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_URL_ARG,        	 grep url_arg        $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VRULE_OR,               grep v_or           $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VRULE_WILDCARD,         grep wildcard       $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VRULE_REHOST,           grep rehost         $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_VRULE_TARGET_IP,        grep target_ip      $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_GEN_EVHOST,             grep evhost         $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_POST_TRACK,             grep post_track     $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_POST_REPORT,            grep post_report    $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_COLLECTOR_RRD,          grep rrd            $conf_h >/dev/null)
# These must be at the end
AM_CONDITIONAL(STATIC_RULE_NOT,               grep _not_          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_AND,               grep _and_          $conf_h >/dev/null)
AM_CONDITIONAL(STATIC_RULE_OR,                grep _or_           $conf_h >/dev/null)

AC_OUTPUT([
Makefile
cget/Makefile
cherokee-config
cherokee.pc
cherokee.spec
org.cherokee.webserver.plist
cherokee/Makefile
contrib/Makefile
icons/Makefile
m4/Makefile
qa/Makefile
doc/Makefile
themes/Makefile
themes/default/Makefile
themes/firefox3/Makefile
themes/white/Makefile
themes/plain/Makefile
packages/Makefile
packages/osx/Makefile
packages/osx/Info.plist
packages/osx/Description.plist
www/Makefile
dbslayer/Makefile
admin/Makefile
admin/icons/Makefile
admin/plugins/Makefile
admin/wizards/Makefile
admin/static/Makefile
admin/static/js/Makefile
admin/static/css/Makefile
admin/static/images/Makefile
admin/static/images/flags/Makefile
admin/static/images/wizards/Makefile
admin/static/images/other/Makefile
admin/CTK/Makefile
admin/CTK/CTK/Makefile
admin/CTK/static/css/Makefile
admin/CTK/static/images/Makefile
admin/CTK/static/js/Makefile
admin/CTK/static/Makefile
po/Makefile
])


methods=""
if test "$have_epoll"        = yes; then methods="${methods}epoll ";  fi
if test "$have_kqueue"       = yes; then methods="${methods}kqueue "; fi
if test "$have_poll"         = yes; then methods="${methods}poll ";   fi
if test "$have_port"         = yes; then methods="${methods}port ";   fi
if test "$have_select"       = yes; then methods="${methods}select";  fi

if test "$use_crypt_r" = "yes"; then
	crypt_type="multithread"
elif test "$use_crypt" = "yes"; then
	crypt_type="single thread"
else
	crypt_type="no"
fi

echo
echo ============================
echo "Install prefix        $prefix"
echo "CFLAGS                $CFLAGS"
echo "trace                 $enable_trace"
echo "backtracing           $enable_backtraces"
echo "sendfile()            $with_sendfile_support"
echo "IPv6 support          $have_ipv6"
i=1
for m in $methods; do
	echo "Polling method $i      $m"
	i=`expr $i + 1`
done
echo "Threading support     $have_pthread"
echo "OpenSSL support       $have_openssl"
echo "PCRE library          $have_pcre"
echo "Compatible PAM        $have_pam"
echo "Python                $have_python"
echo "LDAP                  $have_ldap"
echo "MySQL                 $have_mysql"
echo "GeoIP                 $have_geoip"
echo "FFMpeg                $have_ffmpeg"
echo "crypt support         $crypt_type"
if test "x$is_beta" = "xyes"; then
echo "Beta release          $PACKAGE_PATCH_VERSION"
fi
echo
eval eval echo "Installation dir      $bindir"
echo ============================
echo
if test "x$cherokeepath" != "x" ; then
	echo Warning: You have an old copy of Cherokee at $cherokeepath.
	echo
fi

cat <<THEEND
+------------------------------------------------------------------+
| License:                                                         |
| This software is subject to the GPL License, available in this   |
| distribution in the file COPYING. Please, remember that any copy |
| distribution or modification of the software is subject to it.   |
+------------------------------------------------------------------+

Thank you for using Cherokee.
THEEND
