#!/bin/sh

# Renders a ChangeLog file by using the Subversion or Git commit logs
#
# Author: Alvaro Lopez Ortega <alvaro@alobbs.com>

TZ=UTC

# The current ChangeLog file starts at revision number..
FIRST_REV="5776"
FIRST_ID="0ddb11e3c15767ee447a17174502131db462b2cb"

# Local vars
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
test -z "$SVN" && SVN=svn
test -z "$GIT" && GIT=git


go_git()
{
   # Figure last ChangeLog entry
   LAST_COMMIT_ID=`head -n 3 ChangeLog 2>/dev/null | grep 'svn=' | sed 's/ *svn=.* git=\(.*\)/\1/'`

   if [ "x$LAST_COMMIT_ID" = "x" ]; then
	  LAST_COMMIT_ID="$FIRST_ID"
	  echo " * Creating a brand new ChangeLog file"
   else
	  echo " * Last commit in the ChangeLog: $LAST_COMMIT_ID"
   fi

   # New log
   echo " * Appending commits since: $LAST_COMMIT_ID"
   $GIT log --stat --no-merges --date=short $LAST_COMMIT_ID..HEAD | python $srcdir/gitlog2changelog.py > $srcdir/ChangeLog.new

   if test ! -s $srcdir/ChangeLog.new ; then
	  echo " * Changelog is already up-to-date."
   else
	  if test -f $srcdir/ChangeLog ; then
		 echo " * Merging new entries.."
		 mv $srcdir/ChangeLog $srcdir/ChangeLog.prev
		 cat $srcdir/ChangeLog.new $srcdir/ChangeLog.prev > ChangeLog
	  else
		 echo " * No previous entries.."
		 mv $srcdir/ChangeLog.new $srcdir/ChangeLog
	  fi
   fi

   echo " * Cleaning up"
   rm -f $srcdir/ChangeLog.new $srcdir/ChangeLog.prev
}


go_svn()
{
   # FOR THE RECORD:
   # Commit log messages can be modified by running (replace $REV):
   #  svn propedit svn:log --revprop -r$REV .

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
		 $SVN log -v --xml -r $SVN_VERSION:$((CHANGELOG_VERSION+1)) $srcdir | python $srcdir/svnlog2changelog.py > $srcdir/ChangeLog
		 cat $srcdir/ChangeLog.prev >> $srcdir/ChangeLog
		 rm -f $srcdir/ChangeLog.prev
	  fi
   fi
}


# main
if [ -d .git ]; then
    go_git
elif [ -d .svn ]; then
    go_svn
else
    echo "The ChangeLog file is not being updated."
    echo
fi
