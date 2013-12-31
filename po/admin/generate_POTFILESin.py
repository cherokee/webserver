#!/usr/bin/env python2

# Cherokee POTFILES.in generator
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
# This file is distributed under the GPL2 license.

import os, re

# Configuration
DIRS = [('admin', ['.+\.py$']),
        (os.path.join('admin','wizards'), ['.+\.py$']),
        (os.path.join('admin','plugins'), ['.+\.py$']),
        (os.path.join('admin','CTK','CTK'), ['.+\.py$']),]

def main():
    # Figure a few paths
    orig_dir     = os.getcwd()
    po_admin_dir = os.path.abspath (os.path.dirname(__file__))
    top_srcdir   = os.path.abspath (os.path.join (po_admin_dir, "../.."))

    # Check the files
    pot_files = []
    for dir, filters in DIRS:
        dir_path = os.path.join (top_srcdir, dir)

        # Read the Makefile.am file
        try:
            makefile_am = open(os.path.join(dir_path, "Makefile.am"), 'r').read()
        except:
            continue

        for file in os.listdir (dir_path):
            # Skip the file is Makefile.am doesn't refer to it
            if not file in makefile_am:
                continue

            # Check the file filters as well
            for filter in filters:
                if not re.search (filter, file):
                    continue
                file_path = os.path.join (dir_path, file)
                if "_(" in open(file_path, 'r').read():
                    pot_files.append (file_path[len(top_srcdir)+1:])

    # Print result
    pot_files.sort()
    print ('\n'.join (pot_files)),

if __name__ == "__main__":
    main()
