#!/usr/bin/env python

##
## Cherokee 0.8.x to 0.9.x configuration converter
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

    cont  = "# Converted by 08to09.py \n"
    cont += '\n'.join(tmp)

    f = open(file, "w+")
    f.write (cont)
    f.close()

sources = []
def add_source (in_source):
    global sources

    for n in range(len(sources)):
        if sources[n] == in_source:
            return n+1, False

    sources.append (in_source)
    return len(sources), True

def get_highest_balancer_source (cin, vserver, rule):
    tmp = cin.keys('vserver!%s!rule!%s!handler!balancer!source' %(vserver, rule))
    if tmp:
        tmp = [int(x) for x in tmp]
        tmp.sort()
        return tmp[-1]+1
    else:
        return 1


def convert (fin, fout):
    n   = 1
    cin = Config(fin)

    # Rewrite the 'source' entries
    for v in cin.keys('vserver'):
        for r in cin.keys('vserver!%s!rule'%(v)):
            balancer = cin.get_val ('vserver!%s!rule!%s!handler!balancer'%(v,r))
            if not balancer: 
                continue

            tipe = cin.get_val('vserver!%s!rule!%s!handler!balancer!type'%(v,r))
            del(cin['vserver!%s!rule!%s!handler!balancer!type'%(v,r)])

            for h in cin.keys('vserver!%s!rule!%s!handler!balancer'%(v,r)):
                if h == 'type': continue

                env  = {}
                host = cin.get_val('vserver!%s!rule!%s!handler!balancer!%s!host'%(v,r,h))
                inte = cin.get_val('vserver!%s!rule!%s!handler!balancer!%s!interpreter'%(v,r,h))

                tmp = cin.keys ('vserver!%s!rule!%s!handler!balancer!%s!env'%(v,r,h))
                if tmp:
                    for e in tmp:
                        env[e] = cin.get_val('vserver!%s!rule!%s!handler!balancer!%s!env!%s'%(v,r,h,e))
                        
                sourcen, is_new = add_source ((tipe, host, inte, env))
                if is_new:
                    cin['source!%d!type'%(sourcen)] = tipe
                    cin['source!%d!host'%(sourcen)] = host
                    if inte:
                        cin['source!%d!interpreter'%(sourcen)] = inte
                    if env:
                        for k in env.keys():
                            cin['source!%d!env!%s'%(sourcen,k)] = env[k]

                del(cin['vserver!%s!rule!%s!handler!balancer!%s'%(v,r,h)])

                h = get_highest_balancer_source(cin,v,r)
                cin['vserver!%s!rule!%s!handler!balancer!source!%d' %(v,r,h) ] = str(sourcen)

    # Remove old server encoders
    del (cin['server!encoder'])

    save_result (str(cin), fout)
                

def main ():
    if len(sys.argv) < 3:
        print "USAGE:"
        print " %s /path/cherokee.conf.08 /path/cherokee.conf.09" % (sys.argv[0])
        print
        raise SystemExit

    convert (sys.argv[1], sys.argv[2])
    
if __name__ == "__main__":
    main()
