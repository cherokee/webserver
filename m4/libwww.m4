AC_DEFUN([LIBWWW_READDIR_R_TYPE],[


dnl Check for readdir_r and number of its arguments
dnl Code from libwww/configure.in

AC_MSG_CHECKING(for readdir_r)
if test -z "$ac_cv_readdir_args"; then
    AC_TRY_COMPILE(
    [
#include <sys/types.h>
#include <dirent.h>
    ],
    [
    struct dirent dir, *dirp;
    DIR *mydir;
    dirp = readdir_r(mydir, &dir);
    ], ac_cv_readdir_args=2)
fi
if test -z "$ac_cv_readdir_args"; then
    AC_TRY_COMPILE(
        [
#include <sys/types.h>
#include <dirent.h>
    ],
    [
        struct dirent dir, *dirp;
        DIR *mydir;
        int rc;
        rc = readdir_r(mydir, &dir, &dirp);
    ], ac_cv_readdir_args=3)
fi

AC_ARG_ENABLE([readdir_r],
              AC_HELP_STRING([--disable-readdir_r], [disable readdir_r usage]),
		    want_readdir_r="$enableval",
		    want_readdir_r="yes")

if test "x$want_readdir_r" != "xyes"; then
    AC_MSG_RESULT(disabled)
elif test -z "$ac_cv_readdir_args"; then
    AC_MSG_RESULT(no)
else
    if test "$ac_cv_readdir_args" = 2; then
	 AC_DEFINE(HAVE_READDIR_R_2,1,[readdir_r takes 2 arguments])
    elif test "$ac_cv_readdir_args" = 3; then
	 AC_DEFINE(HAVE_READDIR_R_3,1,[readdir_r takes 3 arguments])
    fi

    AC_MSG_RESULT([$ac_cv_readdir_args arguments])
fi


])