AC_DEFUN([AX_LIB_MYSQL],[
    AC_ARG_WITH([mysql],
        AC_HELP_STRING([--with-mysql=@<:@ARG@:>@],
            [use MySQL client library @<:@default=yes@:>@, optionally specify path to mysql_config]
        ),
        [
        if test "$withval" = "no"; then
            want_mysql="no"
        elif test "$withval" = "yes"; then
            want_mysql="yes"
        else
            want_mysql="yes"
            MYSQL_CONFIG="$withval"
        fi
        ],
        [want_mysql="yes"]
    )

    MYSQL_CFLAGS=""
    MYSQL_LDFLAGS=""
    MYSQL_VERSION=""

    dnl
    dnl Check MySQL libraries (libpq)
    dnl

    if test "$want_mysql" = "yes"; then

        if test -z "$MYSQL_CONFIG" -o test; then
            AC_PATH_PROG([MYSQL_CONFIG], [mysql_config], [no])
        fi

        if test "$MYSQL_CONFIG" != "no"; then
            AC_MSG_CHECKING([for MySQL libraries])

            MYSQL_CFLAGS="`$MYSQL_CONFIG --cflags`"
            MYSQL_LDFLAGS="`$MYSQL_CONFIG --libs`"

            MYSQL_VERSION=`$MYSQL_CONFIG --version`

            found_mysql="yes"
            AC_MSG_RESULT([yes])
        fi
    fi

    dnl
    dnl Detect mysql.h - alo
    dnl
    if test "$want_mysql" = "yes"; then
	  AC_MSG_CHECKING([for mysql.h (using mysql_config --cflags)])

       old_CFLAGS="$CFLAGS"
	  CFLAGS="$CFLAGS $MYSQL_CFLAGS"
	  AC_TRY_COMPILE([
	      #include <mysql.h>
	  ],[
	      int a = 1;
	  ],
	      have_mysql_h=yes,
	      have_mysql_h=no
	  )
	  CFLAGS="$old_CFLAGS"

	  if test "x$have_mysql_h" = "xyes" ; then
	    AC_MSG_RESULT(yes)
	  else
	    AC_MSG_RESULT(no)
	  fi
    fi


    dnl
    dnl Check if required version of MySQL is available
    dnl

    mysql_version_req=ifelse([$1], [], [], [$1])

    if test "$found_mysql" = "yes" -a -n "$mysql_version_req"; then

        AC_MSG_CHECKING([if MySQL version is >= $mysql_version_req])

        dnl Decompose required version string of MySQL
        dnl and calculate its number representation
        mysql_version_req_major=`expr $mysql_version_req : '\([[0-9]]*\)'`
        mysql_version_req_minor=`expr $mysql_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        mysql_version_req_micro=`expr $mysql_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$mysql_version_req_micro" = "x"; then
            mysql_version_req_micro="0"
        fi

        mysql_version_req_number=`expr $mysql_version_req_major \* 1000000 \
                                   \+ $mysql_version_req_minor \* 1000 \
                                   \+ $mysql_version_req_micro`

        dnl Decompose version string of installed MySQL
        dnl and calculate its number representation
        mysql_version_major=`expr $MYSQL_VERSION : '\([[0-9]]*\)'`
        mysql_version_minor=`expr $MYSQL_VERSION : '[[0-9]]*\.\([[0-9]]*\)'`
        mysql_version_micro=`expr $MYSQL_VERSION : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$mysql_version_micro" = "x"; then
            mysql_version_micro="0"
        fi

        mysql_version_number=`expr $mysql_version_major \* 1000000 \
                                   \+ $mysql_version_minor \* 1000 \
                                   \+ $mysql_version_micro`

        mysql_version_check=`expr $mysql_version_number \>\= $mysql_version_req_number`
        if test "$mysql_version_check" = "1"; then
            AC_MSG_RESULT([yes])
        else
            AC_MSG_RESULT([no])
        fi
    fi

    if test "$found_mysql $have_mysql_h $mysql_version_check" = "yes yes 1";
    then
	   have_mysql="yes"
    else
	   have_mysql="no"
    fi

    AM_CONDITIONAL(HAVE_MYSQL, test $have_mysql = "yes")

    AC_SUBST([MYSQL_VERSION])
    AC_SUBST([MYSQL_CFLAGS])
    AC_SUBST([MYSQL_LDFLAGS])
])

