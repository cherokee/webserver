#!/usr/bin/env python2
# -*- coding: utf-8 -*-

# Cherokee PO stats
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
# This file is distributed under the GPL2 license.

import os
import re
import sys

# Check parameters
if len(sys.argv) < 2:
    print "ERROR: %s admin/*.po" %(sys.argv[0])
    raise SystemExit

files = sys.argv[1:]

# Process the PO files
po_list  = []
file_max = max([len(x) for x in files])

for file in files:
    content = open(file, 'r').read()
    pad = ' ' * (file_max - len(file))

    unfinished = len (re.findall(r'msgstr ""\n\n', content, re.M))
    total      = len (re.findall(r'msgid', content, re.M))
    finished   = total - unfinished

    percent_unfin = (100 * unfinished) / float(total)
    percent_fin   = 100 - percent_unfin

    po_list.append ({'file':        file,
                     'percent_fin': percent_fin,
                     'finished':    finished,
                     'total':       total})

# Print the results
po_list.sort (lambda x,y: cmp(x['percent_fin'], y['percent_fin']))
po_list.reverse()

for e in po_list:
    print "%(file)s\t%(percent_fin) 3d%%\t%(finished)d/%(total)d" %(e)
