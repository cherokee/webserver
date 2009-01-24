#!/usr/bin/env python

##
## Cherokee 0.11.x to 0.98.x configuration converter
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

    # Replace 'server!port_tls'
    lines = map (lambda x: x.replace('server!port_tls', 'server!bind!2!tls = 1\n' + 'server!bind!2!port'), lines)

    # Replace 'server!port'
    lines = map (lambda x: x.replace('server!port', 'server!bind!1!port'), lines)

    # Remove 'server!listen'
    lines = filter (lambda x: not x.startswith('server!listen'), lines)

    # Write it down
    f = open (fout, 'w+')
    f.write ('\n'.join(lines))
    f.close()

def main ():
    if len(sys.argv) < 3:
        print "USAGE:"
        print " %s /path/cherokee.conf.011 /path/cherokee.conf.098" % (sys.argv[0])
        print
        raise SystemExit

    convert (sys.argv[1], sys.argv[2])

if __name__ == "__main__":
    main()
