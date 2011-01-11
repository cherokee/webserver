#!/usr/bin/env python

##
## Cherokee SVNlog2Changelog script
##
## Copyright: Alvaro Lopez Ortega <alvaro@alobbs.com>
## Licensed: GPL v2
##

import re
import sys
import time

from sys import stdin
from developers import DEVELOPERS


def get_commits():
    chunk   = ''
    commits = []

    while True:
        line = stdin.readline()
        if not line:
            break
        elif line.startswith("commit"):
            if not chunk:
                chunk += "%s" %(line)
            else:
                commits.append (chunk)
                chunk = "%s" %(line)
        else:
            chunk += "%s" %(line)

    if chunk:
        commits.append(chunk)

    return commits


def format_body (body):
    result     = ""
    prev_blank = False

    for line in body.splitlines():
        if "git-svn-id: svn://" in line:
            continue

        if not line.strip():
            if prev_blank:
                continue
            else:
                result += "%s\n"%(line)
                prev_blank = True
        else:
            result += "%s\n"%(line)
            prev_blank = False

    return result


def do_parse():
    for entry in get_commits():
        commit = re.findall(r"commit (\w+)", entry)[0]
        author = re.findall(r"Author: +(.+) \<", entry)[0]
        date   = re.findall(r"Date: +(.+)", entry)[0]

        if not "git-svn-id:" in entry:
            print >> sys.stderr, "Unpushed commit: %s, %s - %s" % (author, date, commit)
            continue

        svn_id = re.findall(r"git-svn-id: svn://cherokee-project.com/.+@(.+) ", entry)[0]

        header_end = entry.index('\n\n')
        if header_end == -1:
            continue

        print ("%s  %s" %(date, DEVELOPERS[author]))
        print ("            svn=%s git=%s" %(svn_id, commit))
        print ("")
        print (format_body(entry[header_end + 2:]))


if __name__ == "__main__":
    do_parse()
