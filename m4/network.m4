dnl
dnl check for working getaddrinfo()
dnl
dnl Note that if the system doesn't have gai_strerror(), we
dnl can't use getaddrinfo() because we can't get strings
dnl describing the error codes.
dnl
AC_DEFUN([APR_CHECK_WORKING_GETADDRINFO],[
  AC_CACHE_CHECK(for working getaddrinfo, ac_cv_working_getaddrinfo,[
  AC_TRY_RUN( [
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

void main(void) {
    struct addrinfo hints, *ai;
    int error;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo("127.0.0.1", NULL, &hints, &ai);
    if (error) {
        exit(1);
    }
    if (ai->ai_addr->sa_family != AF_INET) {
        exit(1);
    }
    exit(0);
}
],[
  ac_cv_working_getaddrinfo="yes"
],[
  ac_cv_working_getaddrinfo="no"
],[
  ac_cv_working_getaddrinfo="yes"
])])
if test "$ac_cv_working_getaddrinfo" = "yes"; then
  if test "$ac_cv_func_gai_strerror" != "yes"; then
    ac_cv_working_getaddrinfo="no"
  else
    AC_DEFINE(HAVE_GETADDRINFO, 1, [Define if getaddrinfo exists and works well
enough for APR])
  fi
fi
])

dnl
dnl check for working getnameinfo()
dnl
AC_DEFUN([APR_CHECK_WORKING_GETNAMEINFO],[
  AC_CACHE_CHECK(for working getnameinfo, ac_cv_working_getnameinfo,[
  AC_TRY_RUN( [
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void main(void) {
    struct sockaddr_in sa;
    char hbuf[256];
    int error;

    sa.sin_family = AF_INET;
    sa.sin_port = 0;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
#ifdef SIN6_LEN
    sa.sin_len = sizeof(sa);
#endif

    error = getnameinfo((const struct sockaddr *)&sa, sizeof(sa),
                        hbuf, 256, NULL, 0,
                        NI_NUMERICHOST);
    if (error) {
        exit(1);
    } else {
        exit(0);
    }
}
],[
  ac_cv_working_getnameinfo="yes"
],[
  ac_cv_working_getnameinfo="no"
],[
  ac_cv_working_getnameinfo="yes"
])])
if test "$ac_cv_working_getnameinfo" = "yes"; then
  AC_DEFINE(HAVE_GETNAMEINFO, 1, [Define if getnameinfo exists])
fi
])


AC_DEFUN([APR_CHECK_SOCKADDR_IN6],[
AC_CACHE_CHECK(for sockaddr_in6, ac_cv_define_sockaddr_in6,[
AC_TRY_COMPILE([
#include <sys/types.h>
#include <netinet/in.h>
],[
struct sockaddr_in6 sa;
],[
    ac_cv_define_sockaddr_in6=yes
],[
    ac_cv_define_sockaddr_in6=no
])
])

if test "$ac_cv_define_sockaddr_in6" = "yes"; then
  have_sockaddr_in6=1
else
  have_sockaddr_in6=0
fi
])


dnl
dnl Checks to see if struct sockaddr_storage exists
dnl
dnl usage:
dnl
dnl     AC_ACME_SOCKADDR_STORAGE
dnl
dnl results:
dnl
dnl     HAVE_SOCKADDR_STORAGE (defined)
dnl
AC_DEFUN([AC_ACME_SOCKADDR_STORAGE],
    [AC_MSG_CHECKING(if struct sockaddr_storage exists)
    AC_CACHE_VAL(ac_cv_acme_sockaddr_storage,
        AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>],
        [struct sockaddr_storage sas],
        ac_cv_acme_sockaddr_storage=yes,
        ac_cv_acme_sockaddr_storage=no))
    AC_MSG_RESULT($ac_cv_acme_sockaddr_storage)
    if test $ac_cv_acme_sockaddr_storage = yes ; then
            AC_DEFINE(HAVE_SOCKADDR_STORAGE, 1, [HAVE_SOCKADDR_STORAGE])
    fi])

dnl
dnl Checks to see if struct sockaddr_in6 exists
dnl
dnl usage:
dnl
dnl     AC_ACME_SOCKADDR_IN6
dnl
dnl results:
dnl
dnl     HAVE_SOCKADDR_IN6 (defined)
dnl
AC_DEFUN([AC_ACME_SOCKADDR_IN6],
    [AC_MSG_CHECKING(if struct sockaddr_in6 exists)
    AC_CACHE_VAL(ac_cv_acme_sockaddr_in6,
        AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>],
        [struct sockaddr_in6 sa6],
        ac_cv_acme_sockaddr_in6=yes,
        ac_cv_acme_sockaddr_in6=no))
    AC_MSG_RESULT($ac_cv_acme_sockaddr_in6)
    if test $ac_cv_acme_sockaddr_in6 = yes ; then
            AC_DEFINE(HAVE_SOCKADDR_IN6, 1, [HAVE_SOCKADDR_IN6])
    fi])


dnl
dnl Checks to see if struct sockaddr_un exists
dnl
dnl usage:
dnl
dnl     AC_ACME_SOCKADDR_UN
dnl
dnl results:
dnl
dnl     HAVE_SOCKADDR_UN (defined)
dnl
AC_DEFUN([AC_ACME_SOCKADDR_UN],
    [AC_MSG_CHECKING(if struct sockaddr_un exists)
    AC_CACHE_VAL(ac_cv_acme_sockaddr_un,
        AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>],
        [struct sockaddr_un sa6],
        ac_cv_acme_sockaddr_un=yes,
        ac_cv_acme_sockaddr_un=no))
    AC_MSG_RESULT($ac_cv_acme_sockaddr_un)
    if test $ac_cv_acme_sockaddr_un = yes ; then
            AC_DEFINE(HAVE_SOCKADDR_UN, 1, [HAVE_SOCKADDR_UN])
    fi])




dnl ====================================================================
dnl Check that both the C and C++ compilers support __PRETTY_FUNCTION__

AC_DEFUN([ECOS_C_PRETTY_FUNCTION],[
    AC_REQUIRE([AC_PROG_CC])
    AC_REQUIRE([AC_PROG_CXX])

    AC_CACHE_CHECK("for __PRETTY_FUNCTION__ support",ecos_cv_c_pretty_function,[
        AC_LANG_SAVE
        AC_LANG_C
        AC_TRY_LINK(
            [#include <stdio.h>],
            [puts(__PRETTY_FUNCTION__);],
            c_ok="yes",
            c_ok="no"
        )
        AC_LANG_CPLUSPLUS
        AC_TRY_LINK(
            [#include <cstdio>],
            [puts(__PRETTY_FUNCTION__);],
            cxx_ok="yes",
            cxx_ok="no"
        )
        AC_LANG_RESTORE
        if test "${c_ok}" = "yes" -a "${cxx_ok}" = "yes"; then
            ecos_cv_c_pretty_function="yes"
        fi
    ])
    if test "${ecos_cv_c_pretty_function}" = "yes"; then
        AC_DEFINE(HAVE__PRETTY_FUNCTION__, 1, [Have PRETTY_FUNCTION])
    fi
])
