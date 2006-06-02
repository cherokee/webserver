#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=cherokee

#include our own macros
ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I m4"

DIE=0

test -z "$AUTOMAKE" && AUTOMAKE=automake
test -z "$ACLOCAL" && ACLOCAL=aclocal
test -z "$AUTOCONF" && AUTOCONF=autoconf
test -z "$AUTOHEADER" && AUTOHEADER=autoheader
test -z "$LIBTOOL" && LIBTOOL=libtool
test -z "$LIBTOOLIZE" && LIBTOOLIZE=libtoolize

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
        echo "Get ftp://ftp.gnu.org/gnu/libtool/libtool-1.4.tar.gz"
        echo "(or a newer version if it is available)"
        DIE=1
}

($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake installed to compile $PROJECT."
	echo "Get ftp://ftp.cygnus.com/pub/home/tromey/automake-1.2d.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

if test "$DIE" -eq 1; then
	exit 1
fi

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
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

echo "Running $ACLOCAL $ACLOCAL_FLAGS..."
$ACLOCAL $ACLOCAL_FLAGS

# optionally feature autoheader
($AUTOHEADER --version)  < /dev/null > /dev/null 2>&1 && $AUTOHEADER

# run libtoolize ...
echo "Running libtoolize..."
$LIBTOOLIZE --force

echo "Running automake..."
$AUTOMAKE -a $am_opt
$AUTOHEADER
echo "Running autoconf..."
$AUTOCONF
cd $ORIGDIR

$srcdir/configure --enable-maintainer-mode --enable-more-warnings --enable-debug "$@"

echo 
echo "Now type 'make' to compile $PROJECT."
