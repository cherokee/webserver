#!/bin/bash

VERSION="$1"
DISTRO="$2"
COMPILER="gcc-4.0"
TARBALL="cherokee-${VERSION}.tar.gz"
SRC_DIR="cherokee-${VERSION}"

# Install a few packages
echo "deb http://ftp.us.debian.org/debian/ $DISTRO main" >/etc/apt/sources.list
apt-get update
apt-get -f -y install

apt-get -y install $COMPILER make libtool bison flex libpam0g-dev
for i in `seq 3`; do
  apt-get -f -y install
  sleep 1
done

# Unpack the tarball
cd /tmp
echo "Unpacking $TARBALL.."
tar xfz $TARBALL


failed_to_compile() {
    echo "Failed to compile $1"
    exit 1
}

common_compilation() {
    name=$1
    conf=$2
    target_dir="${SRC_DIR}_${name}"

    rm -rf $target_dir
    cp -r $SRC_DIR $target_dir
    cd $target_dir
    ./configure $conf || failed_to_compile $name
    make || failed_to_compile $name
    cd ..
}

# Compile it in the default way
#
common_compilation "default" ""

# Compile it statically
#
common_compilation "static" "--enable-static --enable-static-module=all"

# Compile it again with GNUTLS
#
apt-get -y install libgnutls-dev
common_compilation "gnutls" "--enable-tls=gnutls"
apt-get -y remove libgnutls-dev

# Compile it again with OpenSSL
#
apt-get -y install libssl-dev
common_compilation "openssl" "--enable-tls=openssl"
apt-get -y remove libssl-dev

# Compile u-Cherokee
#
target="uCherokee"
target_dir="${SRC_DIR}_${target}"

cp -r $SRC_DIR $target_dir
cd $target_dir
./configure --disable-pthread --disable-tls --disable-largefile || failed_to_compile $target
cd cherokee
make -f Makefile.embedded || failed_to_compile $target
cd ../..


