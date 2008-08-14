#!/bin/sh

# Layout for cherokee-project.com
ASCIIDOC_HTML="python asciidoc.py --conf-file=web.conf"

for i in ../*.txt
 do
   FILENAME=`echo $i | sed 's/..\///' | sed 's/\(.*\)txt/\1html/' `
   echo $FILENAME
   $ASCIIDOC_HTML -o $FILENAME $i
 done
