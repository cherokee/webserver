#!/usr/bin/env python

# Cherokee QA Tests
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2008 Alvaro Lopez Ortega
# This file is distributed under the GPL license.

import os
import sys
import time
import copy
import signal
import shutil
import thread
import string
import random
import tempfile

from base import *
from conf import *
from help import *

# Configuration parameters
num       = 1
thds      = 1
pause     = 0
tpause    = 0.0
ssl       = False
clean     = True
kill      = True
quiet     = False
valgrind  = None
strace    = False
port      = None
method    = None
nobody    = False
fcgi      = True
log       = False
help      = False
memproc   = False
randomize = False

server    = CHEROKEE_PATH
delay     = SERVER_DELAY

# Make the DocumentRoot directory
www = tempfile.mkdtemp ("cherokee_www")
tmp = www + "/_tmp/"
pid = tmp + "cherokee.pid"
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
    if   p     == '-c': clean     = False
    elif p     == '-k': kill      = False
    elif p     == '-f': fcgi      = False    
    elif p     == '-q': quiet     = True
    elif p     == '-s': ssl       = True
    elif p     == '-x': strace    = True
    elif p     == '-b': nobody    = True
    elif p     == '-l': log       = True
    elif p     == '-h': help      = True
    elif p     == '-o': memproc   = True
    elif p     == '-a': randomize = True
    elif p[:2] == '-n': num       = int(p[2:])
    elif p[:2] == '-t': thds      = int(p[2:])
    elif p[:2] == '-p': port      = int(p[2:])
    elif p[:2] == '-r': delay     = int(p[2:])
    elif p[:2] == '-j': tpause    = float(p[2:])
    elif p[:2] == '-d': pause     = p[2:]
    elif p[:2] == '-m': method    = p[2:]
    elif p[:2] == '-e': server    = p[2:]
    elif p[:2] == '-v': valgrind  = p[2:]
    else:
        help = True

# Print help
if help:
    help_print_parameters()
    sys.exit(1)

# Fix up pause
if type(pause) == types.StringType:
    if len(pause) > 0:
        pause = int(pause)
    else:
        pause = sys.maxint

# Check the interpreters
print "Interpreters"
print_key('PHP',    look_for_php())
print_key('Python', look_for_python())
print

# Set the panic script
panic = CHEROKEE_PANIC

if panic[0] != '/':
    panic = os.path.normpath (os.path.join (os.getcwd(), CHEROKEE_PANIC))

# Configuration file base
CONF_BASE = """
#
# Cherokee QA tests
#
server!port = %d 
server!keepalive = 1 
server!listen = 127.0.0.1
server!panic_action = %s
server!encoder!gzip!allow = txt
server!pid_file = %s
server!module_dir = %s
server!module_deps = %s

vserver!default!document_root = %s
vserver!default!directory_index = test_index.html,test_index.php,/super_test_index.php
vserver!default!rule!1!match = default
vserver!default!rule!1!handler = common
""" % (PORT, panic, pid, CHEROKEE_MODS, CHEROKEE_DEPS, www)

PHP_FCGI = """\
10000!match = extensions
10000!match!extensions = php
10000!match!final = 0
10000!handler = fcgi
10000!handler!balancer = round_robin
10000!handler!balancer!type = interpreter
10000!handler!balancer!local1!host = localhost:%d
10000!handler!balancer!local1!env!PHP_FCGI_CHILDREN = 5
10000!handler!balancer!local1!interpreter = %s -b %d""" % (PHP_FCGI_PORT, look_for_php(), PHP_FCGI_PORT)

PHP_CGI = """\
10000!match = extensions
10000!match!extensions = php
10000!match!final = 0
10000!handler = phpcgi
10000!handler!interpreter = %s""" % (look_for_php())

if fcgi:
    php_ext = PHP_FCGI
else:
    php_ext = PHP_CGI

for php in php_ext.split("\n"):
    CONF_BASE += "vserver!default!rule!%s\n" % (php)

if method:
    CONF_BASE += "server!poll_method = %s" % (method)

if ssl:
    CONF_BASE += """
vserver!default!ssl_certificate_file = %s
vserver!default!ssl_certificate_key_file = %s
vserver!default!ssl_cal_list_file = %s
""" % (SSL_CERT_FILE, SSL_CERT_KEY_FILE, SSL_CA_FILE)

if log:
    CONF_BASE += """
vserver!default!logger = %s
vserver!default!logger!access!type = file
vserver!default!logger!access!filename = %s
vserver!default!logger!error!type = stderr
""" % (LOGGER_TYPE, LOGGER_ACCESS, LOGGER_ERROR)


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
    obj.php_conf = php_ext
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
cfg = CONF_BASE + mod_conf
cfg_file = tempfile.mktemp("cherokee_tmp_cfg")
cfg_fd = open (cfg_file, 'w')
cfg_fd.write (cfg)
cfg_fd.close()

# Launch Cherokee
pid = -1
if port is None:
    pid = os.fork()
    if pid == 0:
        if valgrind != None:
            if valgrind[:3] == 'hel':
                os.execl (VALGRIND_PATH, "valgrind", "--tool=helgrind", server, "-C", cfg_file)
            elif valgrind[:3] == 'cac':
                os.execl (VALGRIND_PATH, "valgrind", "--tool=cachegrind", server, "-C", cfg_file)
            elif valgrind[:3] == 'cal':
                os.execl (VALGRIND_PATH, "valgrind", "--tool=callgrind", "--dump-instr=yes", "--trace-jump=yes", "-v", server, "-C", cfg_file)
            else:
                os.execl (VALGRIND_PATH, "valgrind", "--leak-check=full", "--num-callers=40", "-v", "--leak-resolution=high", server, "-C", cfg_file)
        elif strace:
            os.execl (STRACE_PATH, "strace", server, "-C", cfg_file)            
        else:
            name = server[server.rfind('/') + 1:]
            os.execl (server, name, "-C", cfg_file)
    else:
        print "Server"
        print_key ('PID', str(pid));
        print_key ('Path', CHEROKEE_PATH)
        print_key ('Mods', CHEROKEE_MODS)
        print_key ('Deps', CHEROKEE_DEPS)
        print

        if memproc:
            cmd = 'xterm -e "             \
            ( while :; do                 \
               [ -d /proc/%d ] || break;  \
               date;                      \
               cat /proc/%d/status 2>/dev/null | egrep \'(VmSize|VmData|SleepAVG)\'; \
               echo;                      \
               sleep 1;                   \
            done; ) | less -b1024 -O/tmp/cherokee_mem.txt" &' % (pid, pid)
            os.system(cmd)

        # Count down
        count_down ("Tests will start in %d secs", delay)

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
        print_key ("Testdir", www)
        print_key ("Config",  cfg_file)
        print

    # Kill the server
    if kill and pid > 0:
        print "Sending SIGTERM.."
        os.kill (pid, signal.SIGTERM)
        if valgrind != None:
            linger = 8
        else:
            linger = 4
        count_down ("Will kill the server in %d secs", linger)
        print "Sending SIGKILL.."
        os.kill (pid, signal.SIGKILL)

def mainloop_iterator(objs):
    global port
    global pause
    global its_clean

    time.sleep (.2)

    for n in range(num):
        # Randomize files
        if randomize:
            for n in range(len(objs))*2:
                o = random.randint(0,len(objs)-1)
                t = random.randint(0,len(objs)-1)
                tmp = objs[t]
                objs[t] = objs[o]
                objs[o] = tmp

        # Iterate
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
                print "Success"
                obj.Clean()

            if tpause > 0.0:
                if not quiet:
                    print "Sleeping %2.2f seconds..\r" % (tpause),
                    sys.stdout.flush()
                time.sleep (tpause)

if ssl:
    port = 443

# Maybe launch some threads
for n in range(thds-1):
    objs_copy = map (lambda x: copy.copy(x), objs)

    t = (n * 1.0 / (thds-1))
    time.sleep(t)

    thread.start_new_thread (mainloop_iterator, (objs_copy,))

# Execute the tests
mainloop_iterator(objs)

# It's time to kill Cherokee.. :-(
clean_up()
