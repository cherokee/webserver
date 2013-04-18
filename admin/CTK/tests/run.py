#!/usr/bin/env python

import os
import sys
import tempfile

def write_cherokee_conf (test):
    # Figure out paths
    here = os.path.abspath (os.path.dirname (__file__))
    root = os.path.abspath (os.path.join (here, ".."))

    # Write the custom configuration file
    test_path = "python %s" %(os.path.join (here, test))

    config = open('cherokee.conf.orig', 'r').read()
    config = config.replace ('vserver!10!document_root = /var/www',
                             'vserver!10!document_root = ' + root)
    config = config.replace ('source!1!interpreter = /usr/bin/true',
                             'source!1!interpreter = ' + test_path)
    config = config.replace ('vserver!10!rule!200!document_root = /var/www/static',
                             'vserver!10!rule!200!document_root = %s/static' %(root))

    tempfd, tempname = tempfile.mkstemp()
    os.write(tempfd, config)
    os.close(tempfd)

    # Set environment var PYTHONPATH
    os.putenv("PYTHONPATH", "%s:%s"%(root, os.getenv("PYTHONPATH", '')))

    return tempname


def main (test):
    cherokee_conf = write_cherokee_conf (test)
    command = "cherokee -C %s" %(cherokee_conf)

    print "Running:   %s" % (test)
    print "Executing: %s" % (command)
    print

    os.system(command)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "USAGE:"
        print "  %s test.py" %(sys.argv[0])
        raise SystemExit

    main (sys.argv[1])
