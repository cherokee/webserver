#!/usr/bin/env python2
# -*- coding: utf-8 -*-

# Cherokee QA Tests
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
# This file is distributed under the GPL2 license.

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
from help import *

try:
    # Try to load a local configuration
    from conf_local import *
except ImportError:
    # Use default settings
    from conf import *

# Deals with UTF-8
if sys.getdefaultencoding() != 'utf-8':
    reload (sys)
    sys.setdefaultencoding('utf-8')

# Configuration parameters
num       = 1
thds      = 1
pause     = 0
tpause    = 0.0
bpause    = None
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
proxy     = None

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
    elif p[:2] == '-D': bpause    = p[2:]
    elif p[:2] == '-m': method    = p[2:]
    elif p[:2] == '-e': server    = p[2:]
    elif p[:2] == '-v': valgrind  = p[2:]
    elif p[:2] == '-P': proxy     = p[2:]
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
if php_interpreter:
    print_key('PHP', php_interpreter)
else:
    print_key('PHP', "Couldn't find a suitable PHP interpreter (with fastcgi support)")

if python_interpreter:
    print_key('Python', python_interpreter)
else:
    print_key('Python', "ERROR: Python interpreter not found")
print

# Might need to fake PHP
fake_php = len(php_interpreter) == 0
if fake_php > 0:
    php_interpreter = "false"

# Set the panic script
panic = CHEROKEE_PANIC

if panic[0] != '/':
    panic = os.path.normpath (os.path.join (os.getcwd(), CHEROKEE_PANIC))

# Proxy
if not proxy:
    if ssl:
        client_port = PORT_TLS
    else:
        client_port = PORT

    client_host    = HOST
    listen         = '127.0.0.1'
    proxy_cfg_file = None
else:
    pieces = proxy.split(':')
    if len(pieces) == 2:
        client_host, client_port = pieces
        public_ip = figure_public_ip()
    else:
        client_host, client_port, public_ip = pieces

    listen = ''

    PROXY_CONF = """
server!chunked_encoding = 1
server!keepalive_max_requests = 500
server!log_flush_lapse = 0
server!panic_action = /usr/bin/cherokee-panic
server!bind!1!port = %(client_port)s

vserver!10!nick = default
vserver!10!document_root = /dev/null
vserver!10!logger = combined
vserver!10!logger!access!type = stderr
vserver!10!logger!error!type = stderr

vserver!10!rule!100!match = default
vserver!10!rule!100!match!final = 1
vserver!10!rule!100!handler = proxy
vserver!10!rule!100!handler!balancer = round_robin
vserver!10!rule!100!handler!balancer!source!1 = 1
vserver!10!rule!100!handler!in_preserve_host = 1

source!1!host = %(public_ip)s:%(PORT)s
source!1!nick = QA
source!1!type = host
""" % (locals())

    proxy_cfg_file = tempfile.mktemp("cherokee_proxy_cfg")
    f = open (proxy_cfg_file, 'w')
    f.write (PROXY_CONF)
    f.close()

# Configuration file base
next_source = get_next_source()

CONF_BASE = """
#
# Cherokee QA tests
#
server!bind!1!port = %(PORT)d
server!bind!1!interface = %(listen)s
server!bind!2!port = %(PORT_TLS)d
server!bind!2!interface = %(listen)s
server!bind!2!tls = 1
server!keepalive = 1
server!panic_action = %(panic)s
server!pid_file = %(pid)s
server!module_dir = %(CHEROKEE_MODS)s
server!module_deps = %(CHEROKEE_DEPS)s
server!fdlimit = 8192
server!themes_dir = %(CHEROKEE_THEMES)s

vserver!1!nick = default
vserver!1!document_root = %(www)s
vserver!1!directory_index = test_index.html,test_index.php,/super_test_index.php
vserver!1!rule!1!match = default
vserver!1!rule!1!handler = common

source!%(next_source)d!type = interpreter
source!%(next_source)d!host = localhost:%(PHP_FCGI_PORT)d
source!%(next_source)d!env!PHP_FCGI_CHILDREN = 5
""" % (locals())

if 'fpm' in php_interpreter:
    CONF_BASE += "source!%(next_source)d!interpreter = %(php_interpreter)s -d listen=%(PHP_FCGI_PORT)d -d daemonize=Off\n"%(locals())
else:
    CONF_BASE += "source!%(next_source)d!interpreter = %(php_interpreter)s -b %(PHP_FCGI_PORT)d\n" %(locals())

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
    CONF_BASE = """
server!tls = libssl
""" + CONF_BASE + """
vserver!1!ssl_certificate_file = %(SSL_CERT_FILE)s
vserver!1!ssl_certificate_key_file = %(SSL_CERT_KEY_FILE)s
""" % (globals())

if log:
    CONF_BASE += """
vserver!1!logger = %s
vserver!1!logger!access!type = file
vserver!1!logger!access!filename = %s
""" % (LOGGER_TYPE, LOGGER_ACCESS)

CONF_BASE += """
vserver!1!error_writer!type = stderr
"""

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
    obj.is_ssl   = ssl
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
            valgrind_path = look_for_exec_in_path ("valgrind")

            params = [valgrind_path, "valgrind"]
            if sys.platform == 'darwin':
                params += ['--dsymutil=yes']

            if valgrind[:3] == 'hel':
                params += ["--tool=helgrind"]
            elif valgrind[:3] == 'cac':
                params += ["--tool=cachegrind"]
            elif valgrind[:3] == 'cal':
                params += ["--tool=callgrind", "--dump-instr=yes", "--trace-jump=yes", "-v"]
            else:
                params += ["--leak-check=full", "--num-callers=40", "-v", "--leak-resolution=high"]

            params += [server, "-C", cfg_file]
            os.execl (*params)
        elif strace:
            if sys.platform.startswith('darwin') or \
               sys.platform.startswith('sunos'):
                dtruss_path = look_for_exec_in_path ("dtruss")
                os.execl (dtruss_path, "dtruss", server, "-C", cfg_file)
            else:
                strace_path = look_for_exec_in_path ("strace")
                os.execl (strace_path, "strace", server, "-C", cfg_file)
        else:
            name = server[server.rfind('/') + 1:]

            env = os.environ
            if not env.has_key('CHEROKEE_PANIC_OUTPUT'):
                env['CHEROKEE_PANIC_OUTPUT'] = 'stdout'

            os.execle (server, name, "-C", cfg_file, env)
    else:
        print "Server"
        print_key ('PID', str(pid));
        print_key ('Path',   CHEROKEE_PATH)
        print_key ('Mods',   CHEROKEE_MODS)
        print_key ('Deps',   CHEROKEE_DEPS)
        print_key ('Panic',  CHEROKEE_PANIC)
        print_key ('Themes', CHEROKEE_THEMES)
        if proxy_cfg_file:
            print_key ('Proxy conf', proxy_cfg_file)
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

def mainloop_iterator (objs, main_thread=True):
    global port
    global pause
    global its_clean

    time.sleep (.2)

    for n in range(num):
        # Randomize files
        if randomize:
            random.shuffle(objs)

        # Iterate
        for obj in objs:
            if obj.disabled:
                go_ahead = False
            else:
                go_ahead = obj.Precondition()

            if proxy and not obj.proxy_suitable:
                go_ahead = False

            if go_ahead and main_thread:
                if pause > 0:
                    do_pause()
                elif bpause and obj.file.startswith(bpause):
                    do_pause()

            if not quiet:
                if ssl: print "SSL:",
                print "%s: " % (obj.name) + " "*(40 - len(obj.name)),
                sys.stdout.flush()

            if not go_ahead:
                if not quiet:
                    if obj.disabled:
                        print MESSAGE_DISABLED
                    else:
                        print MESSAGE_SKIPPED
                    if not obj in skipped:
                        skipped.append(obj)
                continue

            try:
                obj.JustBefore(www)
                ret = obj.Run (client_host, client_port)
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


# If we want to pause once do it before launching the threads
if pause == 1:
    do_pause()

# Maybe launch some threads
for n in range(thds-1):
    # Delay launch a little
    delay = ((random.randint(0,50) + len(objs)) / 100.0)
    time.sleep (delay)

    # Launch the thread
    objs_copy = copy.deepcopy(objs)
    thread.start_new_thread (mainloop_iterator, (objs_copy, False))

# Execute the tests
mainloop_iterator(objs)

# We're done. Kill the server.
clean_up()
