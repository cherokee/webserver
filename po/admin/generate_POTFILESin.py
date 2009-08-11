#!/usr/bin/env python

# Cherokee POTFILES.in generator
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009 Alvaro Lopez Ortega
# This file is distributed under the GPL2 license.

# Configuration
DIRS = [('admin', ['.+\.py$'])]


import os, re

def _get_top_srcdir (po_admin_dir):
    return re.findall (r"top_srcdir = (.+)[\r\n]",
                       open("%s/Makefile"%(po_admin_dir)).read())[0]

def main():
    # Figure a few paths
    orig_dir     = os.getcwd()
    po_admin_dir = os.path.abspath (os.path.dirname(__file__))
    top_srcdir   = os.path.abspath (os.path.join (po_admin_dir, _get_top_srcdir(po_admin_dir)))

    # Check the files
    pot_files = []
    for dir, filters in DIRS:
        dir_path = os.path.join (top_srcdir, dir)
        for file in os.listdir (dir_path):
            for filter in filters:
                if not re.search (filter, file):
                    continue
                file_path = os.path.join (dir_path, file)
                if "_(" in open(file_path, 'r').read():
                    pot_files.append (file_path[len(top_srcdir)+1:])

    # Print result
    pot_files.sort()
    print '\n'.join (pot_files),
    
if __name__ == "__main__":
    main()
