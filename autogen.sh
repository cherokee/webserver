#!/bin/sh
# Run this to generate all the initial makefiles, etc.

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

DIE=0

test -z "$AUTOMAKE" && AUTOMAKE=automake
test -z "$ACLOCAL" && ACLOCAL=aclocal
test -z "$AUTOCONF" && AUTOCONF=autoconf
test -z "$AUTOHEADER" && AUTOHEADER=autoheader
test -z "$SVN" && SVN=svn
if [ -x /usr/bin/glibtool ]; then
 test -z "$LIBTOOL" && LIBTOOL=glibtool
 test -z "$LIBTOOLIZE" && LIBTOOLIZE=glibtoolize
else
 test -z "$LIBTOOL" && LIBTOOL=libtool
 test -z "$LIBTOOLIZE" && LIBTOOLIZE=libtoolize
fi


# Build the Changelog file
FIRST_REV="3357"

if [ -e $srcdir/ChangeLog ]; then
    CHANGELOG_VERSION=`head -n 2 $srcdir/ChangeLog | tail -n 1 | awk {'print $1'} | grep ^r[0-9] | sed s/r//g`
else
    touch ChangeLog
    CHANGELOG_VERSION=$FIRST_REV
fi

SVN_VERSION=`svnversion -nc . | sed -e 's/^[^:]*://;s/[A-Za-z]//'`
if [ -z $SVN_VERSION ]; then
     echo
	echo "WARNING: Couldn't get svn revision number."
	echo "         Is svn or the .svn directories missing?"
	echo
else
    if [ $SVN_VERSION -eq $CHANGELOG_VERSION ]; then
	   echo "ChangeLog is already up-to-date."
    else
	   echo "Updating ChangeLog from version $CHANGELOG_VERSION to $SVN_VERSION..."
	   mv $srcdir/ChangeLog $srcdir/ChangeLog.prev
	   TZ=UTC $SVN log -v --xml -r $((CHANGELOG_VERSION+1)):$SVN_VERSION $srcdir | python $srcdir/svnlog2changelog.py > $srcdir/ChangeLog
	   cat $srcdir/ChangeLog.prev >> $srcdir/ChangeLog
	   rm $srcdir/ChangeLog.prev
    fi
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
     m4list="etr_socket_nsl.m4 network.m4 sendfile_samba.m4"

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
if grep "^AM_PROG_LIBTOOL" configure.in >/dev/null; then
  echo "Running: libtoolize --force --copy..."
  $LIBTOOLIZE --force --copy
fi

# Aclocal
echo "Running: $ACLOCAL $ACLOCAL_FLAGS..."
rm -f aclocal.m4
$ACLOCAL $ACLOCAL_FLAGS

# Autoheader
if grep "^AM_CONFIG_HEADER" configure.in >/dev/null; then
  echo "Running: autoheader..."
  $AUTOHEADER
fi

# Automake
echo "Running: automake -a $am_opt..."
$AUTOMAKE -a $am_opt

# Autoconf
echo "Running: autoconf..."
$AUTOCONF

# ./configure
cd $ORIGDIR
$srcdir/configure "$@"

echo 
echo "Now type 'make' to compile $PROJECT."
