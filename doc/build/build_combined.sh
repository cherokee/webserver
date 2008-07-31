#!/bin/sh
PACKAGE_VERSION=`grep "PACKAGE_VERSION = " ../Makefile|  sed 's/\(.*\)PACKAGE_VERSION = //'`

python asciidoc.py --conf-file=combined_template.conf --unsafe -a toc -a toclevels=4 -a numbered -a cherokee_version=${PACKAGE_VERSION} -o ../combined.html combined.txt
