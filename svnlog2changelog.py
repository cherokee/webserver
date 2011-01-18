#!/usr/bin/env python

##
## Cherokee SVNlog2Changelog script
##
## Copyright: Alvaro Lopez Ortega <alvaro@alobbs.com>
## Licensed: GPL v2
##

import time
import xml.dom.minidom

from sys import stdin
from developers import DEVELOPERS


def get_text (nodelist):
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            return node.data

def entry_get_val (entry, key):
    dom_element = entry.getElementsByTagName(key)[0]
    nodelist    = dom_element.childNodes
    return get_text(nodelist)

def reformat_msg (msg):
    if not msg:
        return ''

    txts = []
    for l in msg.split('\n'):
        if not txts:
            txts.append('\t* %s'%(l))
        else:
            txts.append('\t%s'%(l))
    return '\n'.join(txts)

def render_paths (entry):
    txt = ''
    for paths in entry.getElementsByTagName('paths'):
        for path in entry.getElementsByTagName('path'):
            action = path.getAttribute('action')
            txt += "\t%s %s\n"%(action, get_text(path.childNodes))
    return txt

def do_parse():
    try:
        dom = xml.dom.minidom.parseString (stdin.read())
    except xml.parsers.expat.ExpatError, e:
        print ("ERROR: Could update ChangeLog. The XML arser reported: ")
        print ('       "%s2"' %(e))
        print ('')
        raise SystemExit

    log = dom.getElementsByTagName('log')[0]

    for entry in log.getElementsByTagName('logentry'):
        revision = entry.getAttribute('revision')
        date     = entry_get_val (entry, 'date').split('T')[0]
        time     = entry_get_val (entry, 'date').split('T')[1].split('.')[0]
        dev      = entry_get_val (entry, 'author')
        msg      = reformat_msg(entry_get_val (entry, 'msg'))
        author   = DEVELOPERS[dev]
        paths    = render_paths(entry)

        print ("%s  %s" %(str(date), author))
        print (" "*12 + "svn=%s" %(revision))
        print ("")
        if msg:
            print (msg.encode("utf-8")),

        if paths:
            print ("")
            print (paths)

if __name__ == "__main__":
    do_parse()

