#################################################
# check for a PAM clear-text auth, accounts, password and session support
with_pam_for_crypt=no
AC_MSG_CHECKING(whether to use PAM)
AC_ARG_WITH(pam,
[  --with-pam              Include PAM support (default=yes)],
[ case "$withval" in
  no)
    AC_MSG_RESULT(no)
    ;;
  *)
    AC_MSG_RESULT(yes)
    AC_DEFINE(WITH_PAM,1,[Whether to include PAM support])
    AUTHLIBS="$AUTHLIBS -lpam"
    with_pam_for_crypt=yes
    ;;
  esac ],
  AC_MSG_RESULT(no)
)

# we can't build a pam module if we don't have pam.
AC_CHECK_LIB(pam, pam_get_data, [AC_DEFINE(HAVE_LIBPAM,1,[Whether libpam is available])])
