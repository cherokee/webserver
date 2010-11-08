#!/bin/sh

# Grab the big pot file
wget -O cherokee-big.pot http://www.cherokee-project.com/download/trunk/cherokee.pot

# Merge po files
for i in admin/*.po; do
    msgmerge -o $i.new $i cherokee-big.pot && \
    cp $i.new $i && \
    rm $i.new
done

# Clean up
rm cherokee-big.pot
