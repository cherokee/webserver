# Configure paths for Cherokee
# originally by Owen Taylor

dnl AM_PATH_CHEROKEE([MINIMUM-VERSION, [ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]]])
dnl Test for CHEROKEE, and define CHEROKEE_CFLAGS and CHEROKEE_LIBS
dnl
AC_DEFUN([AM_PATH_CHEROKEE],[
AC_SYS_LARGEFILE

dnl
dnl Get the cflags and libraries from the cherokee-config script
dnl
AC_ARG_WITH(cherokee-prefix,[  --with-cherokee-prefix=PFX   Prefix where Cherokee is 
installed (optional)],
            cherokee_prefix="$withval", cherokee_prefix="")
AC_ARG_WITH(cherokee-exec-prefix,[  --with-cherokee-exec-prefix=PFX Exec prefix 
where Cherokee is installed (optional)],
            cherokee_exec_prefix="$withval", cherokee_exec_prefix="")
AC_ARG_ENABLE(cherokeetest, [  --disable-cherokeetest       Do not try to compile 
and run a test Cherokee program],
                    , enable_cherokeetest=yes)

  if test x$cherokee_exec_prefix != x ; then
     cherokee_args="$cherokee_args --exec-prefix=$cherokee_exec_prefix"
     if test x${CHEROKEE_CONFIG+set} != xset ; then
        CHEROKEE_CONFIG=$cherokee_exec_prefix/bin/cherokee-config
     fi
  fi
  if test x$cherokee_prefix != x ; then
     cherokee_args="$cherokee_args --prefix=$cherokee_prefix"
     if test x${CHEROKEE_CONFIG+set} != xset ; then
        CHEROKEE_CONFIG=$cherokee_prefix/bin/cherokee-config
     fi
  fi

  AC_PATH_PROG(CHEROKEE_CONFIG, cherokee-config, no)
  min_cherokee_version=ifelse([$1], ,0.4.18,$1)
  AC_MSG_CHECKING(for CHEROKEE - version >= $min_cherokee_version)
  no_cherokee=""
  if test "$CHEROKEE_CONFIG" = "no" ; then
    no_cherokee=yes
  else
    CHEROKEE_CFLAGS=`$CHEROKEE_CONFIG $cherokeeconf_args --cflags`
    CHEROKEE_LIBS=`$CHEROKEE_CONFIG $cherokeeconf_args --libs`

    cherokee_major_version=`$CHEROKEE_CONFIG $cherokee_args --version | \
		 sed 's/\([[0-9]]\+\)\.\([[0-9]]\+\)\.\([[0-9]]\+\)\(b\?[[0-9]]\+\)/\1/'`
    cherokee_minor_version=`$CHEROKEE_CONFIG $cherokee_args --version | \
		 sed 's/\([[0-9]]\+\)\.\([[0-9]]\+\)\.\([[0-9]]\+\)\(b\?[[0-9]]\+\)/\2/'`
    cherokee_micro_version=`$CHEROKEE_CONFIG $cherokee_config_args --version | \
		 sed 's/\([[0-9]]\+\)\.\([[0-9]]\+\)\.\([[0-9]]\+\)\(b\?[[0-9]]\+\)/\3/'`
    if test "x$enable_cherokeetest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $CHEROKEE_CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $CHEROKEE_LIBS $SDL_LIBS"
dnl
dnl Now check if the installed CHEROKEE is sufficiently new. (Also sanity
dnl checks the results of cherokee-config to some extent
dnl
      rm -f conf.cherokeetest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cherokee.h"

char*
my_strdup (char *str)
{
  char *new_str;

  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;

  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.cherokeetest");
  */
  { FILE *fp = fopen("conf.cherokeetest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_cherokee_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_cherokee_version");
     exit(1);
   }

   if (($cherokee_major_version  > major) ||
      (($cherokee_major_version == major) && ($cherokee_minor_version  > minor)) ||
      (($cherokee_major_version == major) && ($cherokee_minor_version == minor) && ($cherokee_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'cherokee-config --version' returned %d.%d.%d, but the minimum version\n", 
		   $cherokee_major_version, $cherokee_minor_version, $cherokee_micro_version);
      printf("*** of Cherokee required is %d.%d.%d. If cherokee-config is correct, then it is\n",
		   major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If cherokee-config was wrong, set the environment variable CHEROKEE_CONFIG\n");
      printf("*** to point to the correct copy of cherokee-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_cherokee=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_cherokee" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$CHEROKEE_CONFIG" = "no" ; then
       echo "*** The cherokee-config script installed by Cherokee could not be found"
       echo "*** If Cherokee was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the CHEROKEE_CONFIG environment variable to the"
       echo "*** full path to cherokee-config."
     else
       if test -f conf.cherokeetest ; then
        :
       else
          echo "*** Could not run Cherokee test program, checking why..."
          CFLAGS="$CFLAGS $CHEROKEE_CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $CHEROKEE_LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "cherokee.h"
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding Cherokee or finding the wrong"
          echo "*** version of Cherokee. If it is not finding Cherokee, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means Cherokee was incorrectly installed"
          echo "*** or that you have moved Cherokee since it was installed. In the latter case, you"
          echo "*** may want to edit the cherokee-config script: $CHEROKEE_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     CHEROKEE_CFLAGS=""
     CHEROKEE_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(CHEROKEE_CFLAGS)
  AC_SUBST(CHEROKEE_LIBS)
  rm -f conf.cherokeetest
])
