#!/usr/bin/env python

# Cherokee QA Tests
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2006 Alvaro Lopez Ortega
# This file is distributed under the GPL license.

import os
import sys
import time
import copy
import signal
import shutil
import thread
import string
import tempfile

from base import *
from conf import *

# Configuration parameters
num      = 1
thds     = 1
pause    = 0
ssl      = False
clean    = True
kill     = True
quiet    = False
valgrind = False
strace   = False
port     = None
method   = None
nobody   = False
fcgi     = True

server   = CHEROKEE_PATH

# Make the DocumentRoot directory
www = tempfile.mkdtemp ("cherokee_www")
tmp = www + "/_tmp/"
os.makedirs (tmp)

map (lambda x: os.chmod (x, 0777), [www, tmp])

# Make the files list
files = []
param = []
if len(sys.argv) > 1:
    argv = sys.argv[1:]
    files = filter (lambda x: x[0] != '-', argv)
    param = filter (lambda x: x[0] == '-', argv)

# If not files were specified, use all of them
if len(files) == 0:
    files = os.listdir('.') 
    files = filter (lambda x: x[-3:] == '.py', files)
    files = filter (lambda x: x[3] == '-', files)
    files = filter (lambda x: x[2] in string.digits, files)
    files.sort()

# Process the parameters
for p in param:
    if   p     == '-c': clean    = False
    elif p     == '-k': kill     = False
    elif p     == '-f': fcgi     = False    
    elif p     == '-q': quiet    = True
    elif p     == '-v': valgrind = True
    elif p     == '-s': ssl      = True
    elif p     == '-x': strace   = True
    elif p     == '-b': nobody   = True
    elif p[:2] == '-n': num      = int(p[2:])
    elif p[:2] == '-t': thds     = int(p[2:])
    elif p[:2] == '-p': port     = int(p[2:])
    elif p[:2] == '-d': pause    = p[2:]
    elif p[:2] == '-m': method   = p[2:]
    elif p[:2] == '-e': server   = p[2:]

# Fix up pause
if type(pause) == types.StringType:
    if len(pause) > 0:
        pause = int(pause)
    else:
        pause = sys.maxint

# Configuration file base
CONF_BASE = """# Cherokee QA tests
               Port %d
               Keepalive On
               Listen 127.0.0.1
               DocumentRoot %s
               PanicAction /usr/bin/cherokee-panic
               Directory / { Handler common }
               DirectoryIndex test_index.html, test_index.php, /super_test_index.php
               Encoder gzip { allow txt }
               MimeFile /etc/cherokee/mime.types
            """ % (PORT, www)

if fcgi:
    PHP_EXTENSION = """Extension php {
                      Handler fcgi {
                         Server localhost:%d {
                           Env PHP_FCGI_CHILDREN "5"
                           Interpreter "%s -b %d"
                         }
                      }
                  }""" % (PHP_FCGI_PORT, PHPCGI_PATH, PHP_FCGI_PORT)
else:
    PHP_EXTENSION = "Extension php { Handler phpcgi { Interpreter %s } }" % (PHPCGI_PATH)


if ssl:
    CONF_BASE += """SSLCertificateFile    %s
                   SSLCertificateKeyFile %s
                   SSLCAListFile         %s
                 """ % (SSL_CERT_FILE, SSL_CERT_KEY_FILE, SSL_CA_FILE)

if method:
    CONF_BASE += """ PollMethod %s
                 """ % (method)

# Import modules 
mods = []
for f in files:
    mod = importfile (f)
    mods.append (mod)

# Build objects
objs = []
for m in mods:
    obj = m.Test()
    # Update 'base.py': TestCollection if new
    # properties are added here!
    obj.tmp      = tmp
    obj.nobody   = nobody
    obj.php_conf = PHP_EXTENSION
    objs.append(obj)

# Prepare www files
for obj in objs:
    obj.Prepare(www)

# Generate configuration
mod_conf = ""
for obj in objs:
    if obj.conf is not None:
        mod_conf += obj.conf+"\n"

# Write down the configuration file
cfg = CONF_BASE + mod_conf + PHP_EXTENSION
cfg_file = tempfile.mktemp("cherokee_tmp_cfg")
cfg_fd = open (cfg_file, 'w')
cfg_fd.write (cfg)
cfg_fd.close()

# Launch Cherokee
pid = -1
if port is None:
    pid = os.fork()
    if pid == 0:
        if valgrind:
            os.execl (VALGRIND_PATH, "valgrind", "--leak-check=full", "--num-callers=20", "-v", server, "-C", cfg_file)
        elif strace:
            os.execl (STRACE_PATH, "strace", server, "-C", cfg_file)            
        else:
            name = server[server.rfind('/') + 1:]
            os.execl (server, name, "-C", cfg_file)
    else:
        print "PID: %d - %s" % (pid, CHEROKEE_PATH)
        time.sleep(3)

its_clean = False
def clean_up():
    global clean
    global its_clean

    if its_clean: return
    its_clean = True

    # Clean up
    if clean:
        os.unlink (cfg_file)
        shutil.rmtree (www, True)
    else:
        print "Test directory %s" % (www)
        print "Configuration  %s" %(cfg_file)
        print

    # Kill the server
    if kill and pid > 0:
        os.kill (pid, signal.SIGTERM)
        time.sleep (.5)
        os.kill (pid, signal.SIGKILL)

def mainloop_iterator(objs):
    global port
    global pause
    global its_clean

    time.sleep (.2)

    for n in range(num):
        for obj in objs:
            go_ahead = obj.Precondition()

            if go_ahead and pause > 0:
                print "Press <Enter> to continue.."
                sys.stdin.readline()
                pause = pause - 1

            if not quiet:
                if ssl: print "SSL:",
                print "%s: " % (obj.name) + " "*(40 - len(obj.name)),
                sys.stdout.flush()

            if not go_ahead:
                if not quiet:
                    print "Skipped"
                continue
    
            if port is None:
                port = PORT

            try:
                obj.JustBefore(www)
                ret = obj.Run(port, ssl)
                obj.JustAfter(www)
            except Exception, e:
                if not its_clean:
                    print e
                    clean_up()
                sys.exit(1)

            if ret is not 0:
                if not its_clean:
                    print "Failed"
                    print obj
                    clean_up()
                sys.exit(1)
            elif not quiet:
                print "Sucess"
                obj.Clean()


if ssl:
    port = 443

# Maybe launch some threads
for n in range(thds-1):
    objs_copy = map (lambda x: copy.copy(x), objs)
    thread.start_new_thread (mainloop_iterator, (objs_copy,))

# Execute the tests
mainloop_iterator(objs)

# It's time to kill Cherokee.. :-(
clean_up()
