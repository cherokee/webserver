#!/usr/bin/env python

##
## Cherokee 0.98.x to 0.99.x configuration converter
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
    tmp = filter (lambda x: ("!error_handler!" in x) and (not "!url = " in x), lines)
    for l in tmp:
        new_line = l.replace(' = ', '!url = ')
        lines[lines.index(l)] = new_line

    # Write it down
    f = open (fout, 'w+')
    f.write ('\n'.join(lines)+'\n')
    f.close()

def main ():
    if len(sys.argv) < 3:
        print "USAGE:"
        print " %s /path/cherokee.conf.098 /path/cherokee.conf.099" % (sys.argv[0])
        print
        raise SystemExit

    convert (sys.argv[1], sys.argv[2])

if __name__ == "__main__":
    main()
