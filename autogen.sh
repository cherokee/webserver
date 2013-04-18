#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# Exit on error
set -e

# Exit on pipe fails (if possible)
( set -o pipefail 2> /dev/null ) || true

# Basedir
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=cherokee

#include our own macros
if test x"$ACLOCAL_FLAGS" != "x"; then
    ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I m4"
else
    ACLOCAL_FLAGS="-I m4"
fi

# Check for the tools
DIE=0

test -z "$AUTOMAKE" && AUTOMAKE=automake
test -z "$ACLOCAL" && ACLOCAL=aclocal
test -z "$AUTOCONF" && AUTOCONF=autoconf
test -z "$AUTOHEADER" && AUTOHEADER=autoheader

if hash glibtool 2>&-; then
 test -z "$LIBTOOL" && LIBTOOL=glibtool
 test -z "$LIBTOOLIZE" && LIBTOOLIZE=glibtoolize
else
 test -z "$LIBTOOL" && LIBTOOL=libtool
 test -z "$LIBTOOLIZE" && LIBTOOLIZE=libtoolize
fi

($AUTOCONF --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have autoconf installed to compile $PROJECT."
    echo "Download the appropriate package for your distribution,"
    echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
    DIE=1
}

($LIBTOOL --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have libtool installed to compile $PROJECT."
    echo "Download the appropriate package for your distribution, or"
    echo "get http://ftp.gnu.org/gnu/libtool/libtool-2.2.6a.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
}

($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have automake installed to compile $PROJECT."
    echo "Download the appropriate package for your distribution, or"
    echo "get http://ftp.gnu.org/gnu/automake/automake-1.10.2.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
}

if test "$DIE" -eq 1; then
    exit 1
fi

# Update the POTFILES.in
echo "Generating a fresh po/admin/POTFILES.in file.."
po/admin/generate_POTFILESin.py > po/admin/POTFILES.in

# No arguments warning
if test -z "$*"; then
    echo "WARNING: I'm going to run ./configure with no arguments - if you wish "
    echo "to pass any to it, please specify them on the $0 command line."
    echo
fi

case $CC in
*xlc | *xlc\ * | *lcc | *lcc\ *) am_opt=--include-deps;;
esac

if test -z "$ACLOCAL_FLAGS"; then

	acdir=`$ACLOCAL --print-ac-dir`
     m4list="etr_socket_nsl.m4 network.m4 sendfile_samba.m4 pwd_grp.m4 mysql.m4"

	for file in $m4list
	do
		if [ ! -f "$acdir/$file" ]; then
			echo "WARNING: aclocal's directory is $acdir, but..."
			echo "         no file $acdir/$file"
			echo "         You may see fatal macro warnings below."
			echo "         If these files are installed in /some/dir, set the ACLOCAL_FLAGS "
			echo "         environment variable to \"-I /some/dir\", or install"
			echo "         $acdir/$file."
			echo ""
		fi
	done
fi

# Libtool
if grep "^AM_PROG_LIBTOOL" configure.ac >/dev/null; then
  echo "Running: libtoolize --force --copy..."
  $LIBTOOLIZE --force --copy
fi

# Aclocal
echo "Running: $ACLOCAL $ACLOCAL_FLAGS..."
rm -f aclocal.m4
$ACLOCAL $ACLOCAL_FLAGS

# Autoheader
if grep "^AC_CONFIG_HEADERS" configure.ac >/dev/null; then
  echo "Running: autoheader..."
  $AUTOHEADER
fi

# Automake
# --foreign = Do not enforce GNU standards
#             (ChangeLog, NEWS, THANKS, etc. files)
echo "Running: automake -a $am_opt..."
$AUTOMAKE -a --foreign $am_opt

# Autoconf
echo "Running: autoconf..."
$AUTOCONF

# ./configure
if test x$NO_CONFIGURE != x; then
  exit;
fi

cd $ORIGDIR
$srcdir/configure "$@"

echo
echo "Now type 'make' to compile $PROJECT."
