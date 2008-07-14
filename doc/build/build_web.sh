#!/bin/sh
PACKAGE_VERSION=`grep "PACKAGE_VERSION = " ../Makefile|  sed 's/\(.*\)PACKAGE_VERSION = //'`

# Layout for cherokee-project.com
ASCIIDOC_HTML="python asciidoc.py -b xhtml11 --conf-file=website_template.conf -a cherokee_version='${PACKAGE_VERSION}'" 

for i in ../*.txt
 do
   FILENAME=`echo $i | sed 's/..\///' | sed 's/\(.*\)txt/\1html/' `
   $ASCIIDOC_HTML -o $FILENAME $i
 done
