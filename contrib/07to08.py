#!/usr/bin/env python

##
## Cherokee 0.7.x to 0.8.x configuration converter
##
## Copyright: Alvaro Lopez Ortega <alvaro@alobbs.com>
## Licensed: GPL v2
##

import sys

sys.path.append('../admin/')
from config import *


def save_result (content, file):
    tmp = content.split('\n')
    tmp.sort()

    cont  = "# Converted by 07to08.py \n"
    cont += '\n'.join(tmp)

    f = open(file, "w+")
    f.write (cont)
    f.close()

def domain_cmp (x, y):
    if x == 'default':
        return -1
    elif y == 'default':
        return 1
    else:
        return cmp (x[::-1], y[::-1])

def convert (fin, fout):
    n   = 1
    cin = Config(fin)

    # Sort the vservers
    vservers = list(cin['vserver'])
    vservers.sort(domain_cmp)

    for vserver in vservers:
        if vserver == 'default':
            prio = 1
        else:
            n += 1
            prio = n 

        # Rename the virtual server entry
        to = "vserver!%03d" % (prio)
        orig = "vserver!%s" % (vserver)
        cin.rename (orig, to)

        # Add the 'nick' and 'domain' properties
        cin['%s!nick'%(to)] = vserver
        if not cin['%s!domain'%(to)]:
            cin['%s!domain!1'%(to)] = vserver

    save_result (str(cin), fout)
        

def main ():
    if len(sys.argv) < 3:
        print "USAGE:"
        print " %s /path/cherokee.conf.07 /path/cherokee.conf.08" % (sys.argv[0])
        print
        raise SystemExit

    convert (sys.argv[1], sys.argv[2])
    
if __name__ == "__main__":
    main()
