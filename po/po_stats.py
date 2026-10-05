#!/usr/bin/env python3
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
    print(f"ERROR: {sys.argv[0]} admin/*.po")
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
po_list.sort (key=lambda item: item['percent_fin'])
po_list.reverse()

for e in po_list:
    print(f"{e['file']}\t{e['percent_fin']:3.0f}%\t{int(e['finished'])}/{int(e['total'])}")
