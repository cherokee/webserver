#!/bin/bash

# NOTICE: This script is Debian dependent.
#

BASE="./compilation_test/"
DISTRO="sid"
REPOSITORY="http://http.us.debian.org/debian"
CHROOT_SCRIPT="debian_testing_chroot.sh"
VERSION="$1"
TARBALL="cherokee-${VERSION}.tar.gz"

# Some basic checks
if test "x$TARBALL" == "x"; then
    echo "Usage:"
    echo "  $0 x.y.z"
    echo "Eg: $0 0.4.28b1"
    exit
fi

if [ ! -f $TARBALL ]; then
    echo "Error: $TARBALL not found"
    exit
fi

# Clean it up
sudo rm -rf $BASE
mkdir -p $BASE

# Install basic system
cd $BASE
sudo /usr/sbin/debootstrap $DISTRO `pwd` $REPOSITORY
cd ..

# Copy the second part of the script and the tarball
cp $CHROOT_SCRIPT $BASE/tmp/
cp $TARBALL $BASE/tmp/
chmod a+rx $BASE/tmp/$CHROOT_SCRIPT

# Chroot in there
sudo /usr/sbin/chroot $BASE /tmp/$CHROOT_SCRIPT $VERSION $DISTRO

# Print notice
total_size=`sudo du -s $BASE`
echo "The $BASE directory is ${total}Kb, maybe you want to remove it.."
echo
