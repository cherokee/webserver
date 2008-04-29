#!/usr/bin/env python

##
## Cherokee 0.6.x to 0.7.x configuration converter
##
## Copyright: Alvaro Lopez Ortega <alvaro@alobbs.com>
## Licensed: GPL v2
##

import sys

sys.path.append('../admin/')
from config import *

def reparent_child (orig, target):
    if not orig:
        return 

    for p in orig._child:
        if p == 'priority':
            continue
        target[p] = orig._child[p]

def save_result (content, file):
    tmp = content.split('\n')
    tmp.sort()

    cont  = "# Converted by 06to07.py \n"
    cont += '\n'.join(tmp)

    f = open(file, "w+")
    f.write (cont)
    f.close()

def convert (fin, fout):
    cin = Config(fin)

    for vserver in cin['vserver']:
        rules = ConfigNode()

        dirs = cin['vserver!%s!directory'%(vserver)]
        if dirs:
            for _dir in dirs:
                prio = cin.get_val('vserver!%s!directory!%s!priority'%(vserver, _dir))
                if _dir in '/\\':
                    cin['vserver!%s!rule!%s!match!type'%(vserver, prio)] = 'default'
                else:
                    cin['vserver!%s!rule!%s!match!type'     %(vserver, prio)] = 'directory'
                    cin['vserver!%s!rule!%s!match!directory'%(vserver, prio)] = _dir
                reparent_child (cin['vserver!%s!directory!%s'%(vserver, _dir)], 
                                cin['vserver!%s!rule!%s'     %(vserver, prio)])

        exts = cin['vserver!%s!extensions'%(vserver)]
        if exts:
            for ext in exts:
                prio = cin.get_val('vserver!%s!extensions!%s!priority'%(vserver, ext))
                cin['vserver!%s!rule!%s!match!type'      %(vserver, prio)] = 'extensions'
                cin['vserver!%s!rule!%s!match!extensions'%(vserver, prio)] = ext
                reparent_child (cin['vserver!%s!extensions!%s'%(vserver, ext)], 
                                cin['vserver!%s!rule!%s'      %(vserver, prio)])

        reqs = cin['vserver!%s!request'%(vserver)]
        if reqs:
            for req in reqs:
                prio = cin.get_val('vserver!%s!request!%s!priority'%(vserver, req))
                cin['vserver!%s!rule!%s!match!type'   %(vserver, prio)] = 'request'
                cin['vserver!%s!rule!%s!match!request'%(vserver, prio)] = req
                reparent_child (cin['vserver!%s!request!%s'%(vserver, req)], 
                                cin['vserver!%s!rule!%s'   %(vserver, prio)])

        del (cin["vserver!%s!directory" %(vserver)])
        del (cin["vserver!%s!extensions"%(vserver)])
        del (cin["vserver!%s!request"   %(vserver)])

    save_result (str(cin), fout)


def main ():
    convert (sys.argv[1], sys.argv[2])
    
if __name__ == "__main__":
    main()
