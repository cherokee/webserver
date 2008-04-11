#!/usr/bin/env python

##
## Cherokee 0.6.x to 0.7.x configuration converter
##
## Copyright: Alvaro Lopez Ortega <alvaro@alobbs.com>
## Licensed: GPL v2
##

import sys
import string

def convert (fin, fout):
    while True:
        l = fin.readline()
        if not l: 
            return
        if not '!rule!' in l:
            l = l.replace('!directory!/!', '!rule!default!')
            l = l.replace('!directory!',   '!rule!directory!')
            l = l.replace('!extensions!',  '!rule!extensions!')
            l = l.replace('!request!',     '!rule!request!')
        fout.write(l)

def main ():
    convert (sys.stdin, sys.stdout)
    
if __name__ == "__main__":
    main()
