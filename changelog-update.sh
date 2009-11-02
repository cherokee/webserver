#!/bin/sh

# Renders a ChangeLog file by using the Subversion commit logs
#
# Author: Alvaro Lopez Ortega <alvaro@alobbs.com>

# FOR THE RECORD:
# Commit log messages can be modified by running (replace $REV):
#  svn propedit svn:log --revprop -r$REV .

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
test -z "$SVN" && SVN=svn

# The current ChangeLog file starts at revision number..
FIRST_REV="3357"

# Check what the latest revision of the ChangeLog file is
if [ -e $srcdir/ChangeLog ]; then
    CHANGELOG_VERSION=`head -n 2 $srcdir/ChangeLog | tail -n 1 | awk {'print $2'} | sed 's|r||g; s|,||g'`
else
    touch ChangeLog
fi

if [ x$CHANGELOG_VERSION = x ]; then
    CHANGELOG_VERSION=$FIRST_REV
fi

# Find the latest revision in the SVN
SVN_VERSION=`svnversion -c . | sed -e 's/^[^:]*://;s/[A-Za-z]//'`
if [ x$SVN_VERSION = x ]; then
     echo
	echo "WARNING: Couldn't get svn revision number."
	echo "         Is svn or the .svn directories missing?"
	echo
else
    if [ x$SVN_VERSION = x$CHANGELOG_VERSION ]; then
	   echo "ChangeLog is already up-to-date."
    else
	   echo "Updating ChangeLog from version $CHANGELOG_VERSION to $SVN_VERSION..."
	   mv $srcdir/ChangeLog $srcdir/ChangeLog.prev
	   TZ=UTC $SVN log -v --xml -r $SVN_VERSION:$((CHANGELOG_VERSION+1)) $srcdir | python $srcdir/svnlog2changelog.py > $srcdir/ChangeLog
	   cat $srcdir/ChangeLog.prev >> $srcdir/ChangeLog
	   rm $srcdir/ChangeLog.prev
    fi
fi
