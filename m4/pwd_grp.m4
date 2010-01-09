dnl
dnl checks for password entry functions and header files
dnl
AC_DEFUN([FW_CHECK_PWD],
[

HAVE_GETPWNAM_R=""

AC_MSG_CHECKING(for getpwnam_r with 5 parameters)
AC_TRY_LINK([#include <pwd.h>
#include <stdlib.h>],
getpwnam_r(NULL,NULL,NULL,0,NULL);,AC_DEFINE(HAVE_GETPWNAM_R_5,1,Some systems have getpwnam_r) AC_DEFINE(HAVE_GETPWNAM_R,1,
Some systems have getpwnam_r) AC_MSG_RESULT(yes); HAVE_GETPWNAM_R="yes", AC_MSG_RESULT(no))

if ( test -z "$HAVE_GETPWNAM_R" )
then
        AC_MSG_CHECKING(for getpwnam_r with 4 parameters)
        AC_TRY_LINK([#include <pwd.h>
#include <stdlib.h>],
getpwnam_r(NULL,NULL,NULL,0);,AC_DEFINE(HAVE_GETPWNAM_R_4,1,Some systems have getpwnam_r) AC_DEFINE(HAVE_GETPWNAM_R,1,Some 
systems have getpwnam_r) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))
fi

HAVE_GETPWUID_R=""

AC_MSG_CHECKING(for getpwuid_r with 5 parameters)
AC_TRY_LINK([#include <pwd.h>
#include <stdlib.h>],
getpwuid_r(0,NULL,NULL,0,NULL);,AC_DEFINE(HAVE_GETPWUID_R_5,1,Some systems have getpwuid_r) AC_DEFINE(HAVE_GETPWUID_R,1,Som
e systems have getpwuid_r) AC_MSG_RESULT(yes); HAVE_GETPWUID_R="yes", AC_MSG_RESULT(no))

if ( test -z "$HAVE_GETPWUID_R" )
then
        AC_MSG_CHECKING(for getpwuid_r with 4 parameters)
        AC_TRY_LINK([#include <pwd.h>
#include <stdlib.h>],
getpwuid_r(0,NULL,NULL,0);,AC_DEFINE(HAVE_GETPWUID_R_4,1,Some systems have getpwuid_r) AC_DEFINE(HAVE_GETPWUID_R,1,Some sys
tems have getpwuid_r) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))
fi

])



dnl
dnl checks for group entry functions and header files
dnl
AC_DEFUN([FW_CHECK_GRP],
[

HAVE_GETGRNAM_R=""

AC_MSG_CHECKING(for getgrnam_r with 5 parameters)
AC_TRY_LINK([#include <grp.h>
#include <stdlib.h>],
getgrnam_r(NULL,NULL,NULL,0,NULL);,AC_DEFINE(HAVE_GETGRNAM_R_5,1,Some systems have getgrnam_r) AC_DEFINE(HAVE_GETGRNAM_R,1,
Some systems have getgrnam_r) AC_MSG_RESULT(yes); HAVE_GETGRNAM_R="yes", AC_MSG_RESULT(no))

if ( test -z "$HAVE_GETGRNAM_R" )
then
        AC_MSG_CHECKING(for getgrnam_r with 4 parameters)
        AC_TRY_LINK([#include <grp.h>
#include <stdlib.h>],
getgrnam_r(NULL,NULL,NULL,0);,AC_DEFINE(HAVE_GETGRNAM_R_4,1,Some systems have getgrnam_r) AC_DEFINE(HAVE_GETGRNAM_R,1,Some 
systems have getgrnam_r) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))
fi

HAVE_GETGRGID_R=""

AC_MSG_CHECKING(for getgrgid_r with 5 parameters)
AC_TRY_LINK([#include <grp.h>
#include <stdlib.h>],
getgrgid_r(0,NULL,NULL,0,NULL);,AC_DEFINE(HAVE_GETGRGID_R_5,1,Some systems have getgrgid_r) AC_DEFINE(HAVE_GETGRGID_R,1,Som
e systems have getgrgid_r) AC_MSG_RESULT(yes); HAVE_GETGRGID_R="yes", AC_MSG_RESULT(no))

if ( test -z "$HAVE_GETGRGID_R" )
then
        AC_MSG_CHECKING(for getgrgid_r with 4 parameters)
        AC_TRY_LINK([#include <grp.h>
#include <stdlib.h>],
getgrgid_r(0,NULL,NULL,0);,AC_DEFINE(HAVE_GETGRGID_R_4,1,Some systems have getgrgid_r) AC_DEFINE(HAVE_GETGRGID_R,1,Some sys
tems have getgrgid_r) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))
fi

])

