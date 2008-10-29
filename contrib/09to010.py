#!/usr/bin/env python

##
## Cherokee 0.9.x to 0.10.x configuration converter
##
## Copyright: Alvaro Lopez Ortega <alvaro@alobbs.com>
## Licensed: GPL v2
##

import sys

def convert (fin, fout):
    # Open file
    f = open(fin, 'r')
    lines = [x.strip() for x in f.readlines()]
    f.close()

    # Replace 'elapse' by 'lapse'
    lines = map (lambda x: x.replace('!elapse', '!lapse'), lines)

    # Replace duplicate mime entries
    lines = map (lambda x: x.replace('midi,kar,mpga,mpega', 'midi,kar,mpega'), lines)
    lines = map (lambda x: x.replace('mpeg3,mp2,dl', 'mpeg3,dl'), lines)
    lines = map (lambda x: x.replace('gsm,m3u,wma,wax', 'gsm,wma,wax'), lines)

    # Write it down
    f = open (fout, 'w+')
    f.write ('\n'.join(lines))
    f.close()

def main ():
    if len(sys.argv) < 3:
        print "USAGE:"
        print " %s /path/cherokee.conf.09 /path/cherokee.conf.010" % (sys.argv[0])
        print
        raise SystemExit

    convert (sys.argv[1], sys.argv[2])

if __name__ == "__main__":
    main()
