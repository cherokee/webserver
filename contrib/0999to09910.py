#!/usr/bin/env python

##
## Cherokee 0.99.10 to >=0.99.10 configuration converter
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

    cont  = "# Converted by 0999to09910.py \n"
    cont += '\n'.join(tmp)

    f = open(file, "w+")
    f.write (cont)
    f.close()

def convert (fin, fout):
    cin = Config(fin)

    for v in cin.keys('vserver'):
        domains = cin.keys('vserver!%s!domain'%(v))
        if not domains:
            continue
        for d in domains:
            key = 'vserver!%s!domain!%s'%(v,d)
            dom = cin.get_val(key)
            del (cin[key])
            cin['vserver!%s!match!domain!%s'%(v,d)] = dom
        cin['vserver!%s!match'%(v)] = 'wildcard'

    save_result (str(cin), fout)

def main ():
    if len(sys.argv) < 3:
        print "USAGE:"
        print " %s /path/cherokee.conf.0.99.9 /path/cherokee.conf.0.99.10" % (sys.argv[0])
        print
        raise SystemExit

    convert (sys.argv[1], sys.argv[2])

if __name__ == "__main__":
    main()
