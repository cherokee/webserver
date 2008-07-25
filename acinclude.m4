AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: threads are created detached by default
        # and the JOINABLE attribute has a nonstandard name (UNDETACHED).
        AC_MSG_CHECKING([for joinable pthread attribute])
        AC_TRY_LINK([#include <pthread.h>],
                    [int attr=PTHREAD_CREATE_JOINABLE;],
                    ok=PTHREAD_CREATE_JOINABLE, ok=unknown)
        if test x"$ok" = xunknown; then
                AC_TRY_LINK([#include <pthread.h>],
                            [int attr=PTHREAD_CREATE_UNDETACHED;],
                            ok=PTHREAD_CREATE_UNDETACHED, ok=unknown)
        fi
        if test x"$ok" != xPTHREAD_CREATE_JOINABLE; then
                AC_DEFINE(PTHREAD_CREATE_JOINABLE, $ok,
                          [Define to the necessary symbol if this constant
                           uses a non-standard name on your system.])
        fi
        AC_MSG_RESULT(${ok})
        if test x"$ok" = xunknown; then
                AC_MSG_WARN([we do not know how to create joinable pthreads])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
                *-aix* | *-freebsd*)     flag="-D_THREAD_SAFE";;
                *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
                PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with cc_r
        AC_CHECK_PROG(PTHREAD_CC, cc_r, cc_r, ${CC})
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD




AC_DEFUN([SAMBA_SENDFILE], [

#################################################
# check for sendfile support

with_sendfile_support=yes
AC_MSG_CHECKING(whether to check to support sendfile)
AC_ARG_WITH(sendfile-support,
[  --with-sendfile-support Check for sendfile support (default=yes)],
[ case "$withval" in
  yes)

        AC_MSG_RESULT(yes);

        case "$host_os" in
        *linux*)
                AC_CACHE_CHECK([for linux sendfile64 support],samba_cv_HAVE_SENDFILE64,[
                AC_TRY_LINK([#include <sys/sendfile.h>],
[\
int tofd, fromfd;
off64_t offset;
size_t total;
ssize_t nwritten = sendfile64(tofd, fromfd, &offset, total);
],
samba_cv_HAVE_SENDFILE64=yes,samba_cv_HAVE_SENDFILE64=no)])

                AC_CACHE_CHECK([for linux sendfile support],samba_cv_HAVE_SENDFILE,[
                AC_TRY_LINK([#include <sys/sendfile.h>],
[\
int tofd, fromfd;
off_t offset;
size_t total;
ssize_t nwritten = sendfile(tofd, fromfd, &offset, total);
],
samba_cv_HAVE_SENDFILE=yes,samba_cv_HAVE_SENDFILE=no)])

# Try and cope with broken Linux sendfile....
                AC_CACHE_CHECK([for broken linux sendfile support],samba_cv_HAVE_BROKEN_LINUX_SENDFILE,[
                AC_TRY_LINK([\
#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
#undef _FILE_OFFSET_BITS
#endif
#include <sys/sendfile.h>],
[\
int tofd, fromfd;
off_t offset;
size_t total;
ssize_t nwritten = sendfile(tofd, fromfd, &offset, total);
],
samba_cv_HAVE_BROKEN_LINUX_SENDFILE=yes,samba_cv_HAVE_BROKEN_LINUX_SENDFILE=no)])

        if test x"$samba_cv_HAVE_SENDFILE64" = x"yes"; then
                AC_DEFINE(HAVE_SENDFILE64,1,[Whether 64-bit sendfile() is available])
                AC_DEFINE(LINUX_SENDFILE_API,1,[Whether linux sendfile() API is available])
                AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile() should be used])
        elif test x"$samba_cv_HAVE_SENDFILE" = x"yes"; then
                AC_DEFINE(HAVE_SENDFILE,1,[Whether sendfile() is available])
                AC_DEFINE(LINUX_SENDFILE_API,1,[Whether linux sendfile() API is available])
                AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile() should be used])
        elif test x"$samba_cv_HAVE_BROKEN_LINUX_SENDFILE" = x"yes"; then
                AC_DEFINE(LINUX_BROKEN_SENDFILE_API,1,[Whether (linux) sendfile() is broken])
                AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile should be used])
        else
                AC_MSG_RESULT(no);
        fi

        ;;
        *freebsd*)
                AC_CACHE_CHECK([for freebsd sendfile support],samba_cv_HAVE_SENDFILE,[
                AC_TRY_LINK([\
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>],
[\
        int fromfd, tofd, ret, total=0;
        off_t offset, nwritten;
        struct sf_hdtr hdr;
        struct iovec hdtrl;
        hdr.headers = &hdtrl;
        hdr.hdr_cnt = 1;
        hdr.trailers = NULL;
        hdr.trl_cnt = 0;
        hdtrl.iov_base = NULL;
        hdtrl.iov_len = 0;
        ret = sendfile(fromfd, tofd, offset, total, &hdr, &nwritten, 0);
],
samba_cv_HAVE_SENDFILE=yes,samba_cv_HAVE_SENDFILE=no)])

        if test x"$samba_cv_HAVE_SENDFILE" = x"yes"; then
                AC_DEFINE(HAVE_SENDFILE,1,[Whether sendfile() support is available])
                AC_DEFINE(FREEBSD_SENDFILE_API,1,[Whether the FreeBSD sendfile() API is available])
                AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile() support should be included])
        else
                AC_MSG_RESULT(no);
        fi
        ;;

        *hpux*)
                AC_CACHE_CHECK([for hpux sendfile64 support],samba_cv_HAVE_SENDFILE64,[
                AC_TRY_LINK([\
#include <sys/socket.h>
#include <sys/uio.h>],
[\
        int fromfd, tofd;
        size_t total=0;
        struct iovec hdtrl[2];
        ssize_t nwritten;
        off64_t offset;

        hdtrl[0].iov_base = 0;
        hdtrl[0].iov_len = 0;

        nwritten = sendfile64(tofd, fromfd, offset, total, &hdtrl[0], 0);
],
samba_cv_HAVE_SENDFILE64=yes,samba_cv_HAVE_SENDFILE64=no)])
        if test x"$samba_cv_HAVE_SENDFILE64" = x"yes"; then
                AC_DEFINE(HAVE_SENDFILE64,1,[Whether sendfile64() is available])
                AC_DEFINE(HPUX_SENDFILE_API,1,[Whether the hpux sendfile() API is available])
                AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile() support should be included])
        else
                AC_MSG_RESULT(no);
        fi

                AC_CACHE_CHECK([for hpux sendfile support],samba_cv_HAVE_SENDFILE,[
                AC_TRY_LINK([\
#include <sys/socket.h>
#include <sys/uio.h>],
[\
        int fromfd, tofd;
        size_t total=0;
        struct iovec hdtrl[2];
        ssize_t nwritten;
        off_t offset;

        hdtrl[0].iov_base = 0;
        hdtrl[0].iov_len = 0;

        nwritten = sendfile(tofd, fromfd, offset, total, &hdtrl[0], 0);
],
samba_cv_HAVE_SENDFILE=yes,samba_cv_HAVE_SENDFILE=no)])
        if test x"$samba_cv_HAVE_SENDFILE" = x"yes"; then
                AC_DEFINE(HAVE_SENDFILE,1,[Whether sendfile() is available])
                AC_DEFINE(HPUX_SENDFILE_API,1,[Whether the hpux sendfile() API is available])
                AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile() support should be included])
        else
                AC_MSG_RESULT(no);
        fi
        ;;

        *solaris*)
                AC_CHECK_LIB(sendfile,sendfilev)
                AC_CACHE_CHECK([for solaris sendfilev64 support],samba_cv_HAVE_SENDFILEV64,[
                AC_TRY_LINK([\
#include <sys/sendfile.h>],
[\
        int sfvcnt;
        size_t xferred;
        struct sendfilevec vec[2];
        ssize_t nwritten;
        int tofd;

        sfvcnt = 2;

        vec[0].sfv_fd = SFV_FD_SELF;
        vec[0].sfv_flag = 0;
        vec[0].sfv_off = 0;
        vec[0].sfv_len = 0;

        vec[1].sfv_fd = 0;
        vec[1].sfv_flag = 0;
        vec[1].sfv_off = 0;
        vec[1].sfv_len = 0;
        nwritten = sendfilev64(tofd, vec, sfvcnt, &xferred);
],
samba_cv_HAVE_SENDFILEV64=yes,samba_cv_HAVE_SENDFILEV64=no)])

        if test x"$samba_cv_HAVE_SENDFILEV64" = x"yes"; then
                AC_DEFINE(HAVE_SENDFILEV64,1,[Whether sendfilev64() is available])
                AC_DEFINE(SOLARIS_SENDFILE_API,1,[Whether the soloris sendfile() API is available])
                AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile() support should be included])
        else
                AC_MSG_RESULT(no);
        fi

                AC_CACHE_CHECK([for solaris sendfilev support],samba_cv_HAVE_SENDFILEV,[
                AC_TRY_LINK([\
#include <sys/sendfile.h>],
[\
        int sfvcnt;
        size_t xferred;
        struct sendfilevec vec[2];
        ssize_t nwritten;
        int tofd;

        sfvcnt = 2;

        vec[0].sfv_fd = SFV_FD_SELF;
        vec[0].sfv_flag = 0;
        vec[0].sfv_off = 0;
        vec[0].sfv_len = 0;

        vec[1].sfv_fd = 0;
        vec[1].sfv_flag = 0;
        vec[1].sfv_off = 0;
        vec[1].sfv_len = 0;
        nwritten = sendfilev(tofd, vec, sfvcnt, &xferred);
],
samba_cv_HAVE_SENDFILEV=yes,samba_cv_HAVE_SENDFILEV=no)])

        if test x"$samba_cv_HAVE_SENDFILEV" = x"yes"; then
                AC_DEFINE(HAVE_SENDFILEV,1,[Whether sendfilev() is available])
                AC_DEFINE(SOLARIS_SENDFILE_API,1,[Whether the solaris sendfile() API is available])
                AC_DEFINE(WITH_SENDFILE,1,[Whether to include sendfile() support])
        else
                AC_MSG_RESULT(no);
        fi
        ;;

        *)
        ;;
        esac
        ;;
  *)
    AC_MSG_RESULT(no)
    ;;
  esac ],
  AC_MSG_RESULT(yes)
)

])




AC_DEFUN([HYDRA_TCP_CORK], [

AC_MSG_CHECKING([whether TCP_CORK is a valid TCP socket option])
AC_TRY_COMPILE(
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
,[
  int one = 1, fd;
  if (setsockopt(fd, IPPROTO_TCP, TCP_CORK,
                    (void *) &one, sizeof (one)) == -1)
      return -1;
  return 0;

],
dnl *** FOUND
AC_DEFINE( HAVE_TCP_CORK, 1, [TCP_CORK was found and will be used])
AC_MSG_RESULT(yes),
dnl *** NOT FOUND
AC_MSG_RESULT(no)
)

])

AC_DEFUN([HYDRA_TCP_NOPUSH], [

AC_MSG_CHECKING([whether TCP_NOPUSH is a valid TCP socket option])
AC_TRY_COMPILE(
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
,[
  int one = 1, fd;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH,
                    (void *) &one, sizeof (one)) == -1)
      return -1;
  return 0;

],
dnl *** FOUND
AC_DEFINE( HAVE_TCP_NOPUSH, 1, [TCP_NOPUSH was found and will be used])
AC_MSG_RESULT(yes),
dnl *** NOT FOUND
AC_MSG_RESULT(no)
)

])