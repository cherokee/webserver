#!/usr/bin/env python2

# Cherokee Doc: Link checker
#
# Authors:
#      Taher Shihadeh <taher@unixwars.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
# This file is distributed under the GPL2 license.

import os
import sys
import re

def get_link_refs():
    link_refs = {}
    for f in filter(lambda x: x.endswith('.txt'), os.listdir('.')):
        tmp = re.findall ('link:([^#].+?)(#.*?)?\[', open(f, 'r').read(), re.M)
        for ref in tmp:
            link_refs[ref[0]] = True
    return link_refs.keys()

def get_txt_refs (link_refs):
    """Discard external links, and return the reference to the source .txt of a link"""
    def is_txt (x):
        if not ('://' in x or x.startswith('mailto:')):
            return True

    refs = filter(is_txt, link_refs)
    return [x.replace('.html', '.txt') for x in refs]

def check_links():
    error = False

    refs      = get_link_refs()
    txt_refs  = get_txt_refs (refs)
    txt_files = filter(lambda x: x.endswith('.txt'), os.listdir('.'))

    for ref in txt_refs:
        if not ref in txt_files:
            print "ERROR: %s: File not found" %(ref)
            error = True

    for txt in txt_files:
        if not txt in txt_refs:
            print "ERROR: %s: No longer used" %(txt)
            error = True

    automake_am = open("Makefile.am", 'r').read()
    for txt in txt_refs:
        if not txt.replace('.txt', '.html') in automake_am:
            print "ERROR: %s isn't covered in Makefile.am " %(txt)
            error = True

    # TODO: check if external links are still accessible
    # http_refs = [x for x in refs if x.startswith('http')]

    return error

if __name__ == "__main__":
    # Exit if .txt files are not included
    if not filter (lambda x: x.endswith('.txt'), os.listdir('.')):
        print "Nothing to check.."
        raise SystemExit

    # Test
    error = check_links()
    sys.exit(int(error))
