#!/usr/bin/env python

import os
import sys
import tempfile
import subprocess

def write_cherokee_conf (test):
    # Figure out paths
    here = os.path.abspath (os.path.dirname (__file__))
    root = os.path.abspath (os.path.join (here, ".."))

    # Write the custom configuration file
    test_path = f"python {os.path.join(here, test)}"

    config = open('cherokee.conf.orig', 'r').read()
    config = config.replace ('vserver!10!document_root = /var/www',
                             'vserver!10!document_root = ' + root)
    config = config.replace ('source!1!interpreter = /usr/bin/true',
                             'source!1!interpreter = ' + test_path)
    config = config.replace ('vserver!10!rule!200!document_root = /var/www/static',
                             f'vserver!10!rule!200!document_root = {root}/static')

    tempfd, tempname = tempfile.mkstemp()
    os.write(tempfd, config.encode())
    os.close(tempfd)

    # Set environment var PYTHONPATH
    os.putenv("PYTHONPATH", f"{root}:{os.getenv('PYTHONPATH', '')}")

    return tempname


def main (test):
    cherokee_conf = write_cherokee_conf (test)
    command = f"cherokee -C {cherokee_conf}"

    print(f"Running:   {test}")
    print(f"Executing: {command}")
    print()

    subprocess.run(command, shell=True, check=False)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("USAGE:")
        print(f"  {sys.argv[0]} test.py")
        raise SystemExit

    main (sys.argv[1])
