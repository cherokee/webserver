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
srv_thds  = None
ssl       = False
clean     = True
kill      = True
quiet     = False
valgrind  = None
strace    = False
port      = None
method    = None
nobody    = False
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
files   = []
param   = []
skipped = []
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
    elif p[:2] == '-T': srv_thds  = int(p[2:])    
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

# Check threads and pauses
if thds > 1 and pause > 1:
    print "ERROR: -d and -t are incompatible with each other."
    sys.exit(1)

# Check the interpreters
php_interpreter    = look_for_php()
python_interpreter = look_for_python()

print "Interpreters"
print_key('PHP',    php_interpreter)
print_key('Python', python_interpreter)
print

# Set the panic script
panic = CHEROKEE_PANIC

if panic[0] != '/':
    panic = os.path.normpath (os.path.join (os.getcwd(), CHEROKEE_PANIC))

# Configuration file base
next_source = get_next_source()

CONF_BASE = """
#
# Cherokee QA tests
#
server!port = %(PORT)d
server!port_tls = %(PORT_TLS)d
server!keepalive = 1 
server!listen = 127.0.0.1
server!panic_action = %(panic)s
server!pid_file = %(pid)s
server!module_dir = %(CHEROKEE_MODS)s
server!module_deps = %(CHEROKEE_DEPS)s
server!fdlimit = 8192

vserver!1!nick = default
vserver!1!document_root = %(www)s
vserver!1!directory_index = test_index.html,test_index.php,/super_test_index.php
vserver!1!rule!1!match = default
vserver!1!rule!1!handler = common

source!%(next_source)d!type = interpreter
source!%(next_source)d!host = localhost:%(PHP_FCGI_PORT)d
source!%(next_source)d!env!PHP_FCGI_CHILDREN = 5
source!%(next_source)d!interpreter = %(php_interpreter)s -b %(PHP_FCGI_PORT)d
""" % (locals())

php_ext = """\
10000!match = extensions
10000!match!extensions = php
10000!match!final = 0
10000!handler = fcgi
10000!handler!balancer = round_robin
10000!handler!balancer!source!1 = %(next_source)d\
""" % (locals())

for php in php_ext.split("\n"):
    CONF_BASE += "vserver!1!rule!%s\n" % (php)

if method:
    CONF_BASE += "server!poll_method = %s" % (method)

if ssl:
    CONF_BASE += """
vserver!1!ssl_certificate_file = %s
vserver!1!ssl_certificate_key_file = %s
vserver!1!ssl_ca_list_file = %s
""" % (SSL_CERT_FILE, SSL_CERT_KEY_FILE, SSL_CA_FILE)

if log:
    CONF_BASE += """
vserver!1!logger = %s
vserver!1!logger!access!type = file
vserver!1!logger!access!filename = %s
vserver!1!logger!error!type = stderr
""" % (LOGGER_TYPE, LOGGER_ACCESS, LOGGER_ERROR)

if srv_thds:
    CONF_BASE += """
server!thread_number = %d
""" % (srv_thds)

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
    if obj.Precondition():
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
            if sys.platform.startswith('darwin') or \
               sys.platform.startswith('sunos'):
                os.execl (DTRUSS_PATH, "dtruss", server, "-C", cfg_file)
            else:
                os.execl (STRACE_PATH, "strace", server, "-C", cfg_file)            
        else:
            name = server[server.rfind('/') + 1:]

            env = os.environ
            if not env.has_key('CHEROKEE_PANIC_OUTPUT'):
                env['CHEROKEE_PANIC_OUTPUT'] = 'stdout'

            os.execle (server, name, "-C", cfg_file, env)
    else:
        print "Server"
        print_key ('PID', str(pid));
        print_key ('Path',  CHEROKEE_PATH)
        print_key ('Mods',  CHEROKEE_MODS)
        print_key ('Deps',  CHEROKEE_DEPS)
        print_key ('Panic', CHEROKEE_PANIC)
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

    print

    # Clean up
    if clean:
        os.unlink (cfg_file)
        shutil.rmtree (www, True)
    else:
        print_key ("Testdir", www)
        print_key ("Config",  cfg_file)
        print

    # Skipped tests
    if not quiet:
        print "Skipped tests: %d" % (len(skipped))

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

def do_pause():
    global pause
    print "Press <Enter> to continue.."
    sys.stdin.readline()
    pause = pause - 1

def mainloop_iterator(objs, main_thread=True):
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

            if go_ahead and pause > 0 and main_thread:
                do_pause()

            if not quiet:
                if ssl: print "SSL:",
                print "%s: " % (obj.name) + " "*(40 - len(obj.name)),
                sys.stdout.flush()

            if not go_ahead:
                if not quiet:
                    print MESSAGE_SKIPPED
                    if not obj in skipped:
                        skipped.append(obj)
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
                    print_sec(obj)
                    clean_up()
                sys.exit(1)

            if ret is not 0:
                if not its_clean:
                    print MESSAGE_FAILED
                    print_sec (obj)
                    clean_up()
                sys.exit(1)
            elif not quiet:
                print MESSAGE_SUCCESS
                obj.Clean()

            if main_thread and tpause > 0.0:
                if not quiet:
                    print "Sleeping %2.2f seconds..\r" % (tpause),
                    sys.stdout.flush()
                time.sleep (tpause)


if ssl:
    port = PORT_TLS

# If we want to pause once do it before launching the threads
if pause == 1:
    do_pause()

# Maybe launch some threads
for n in range(thds-1):
    time.sleep (random.randint(0,50) / 100.0)
    thread.start_new_thread (mainloop_iterator, (copy.deepcopy(objs), False))

# Execute the tests
mainloop_iterator(objs)

# It's time to kill Cherokee.. :-(
clean_up()
