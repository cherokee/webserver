#!/usr/bin/env python2

# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

# Remote Debugging
# Uncomment the following two lines to enable remote debugging
#import pydevd
#pydevd.settrace('localhost', port=9091, stdoutToServer=True, stderrToServer=True)

# System
import os
import sys
import stat
import socket
import signal
import thread

# Import CTK
sys.path.append (os.path.abspath (os.path.realpath(__file__) + '/../CTK'))
import CTK

# Cherokee imports
import config_version
from configured import *
import PageError


def init (scgi_port, cfg_file):
    # Translation support
    CTK.i18n.install ('cherokee', LOCALEDIR, unicode=True)

    # Ensure SIGCHLD is set. It needs to receive the signal in order
    # to detect when its child processes finish.
    if signal.getsignal (signal.SIGCHLD) == signal.SIG_IGN:
        signal.signal (signal.SIGCHLD, signal.SIG_DFL)

    # Move to the server directory
    pathname, scriptname = os.path.split(sys.argv[0])
    os.chdir(os.path.abspath(pathname))

    # Let the user know what is going on
    version = VERSION
    pid     = os.getpid()

    if scgi_port.isdigit():
        print _("Server %(version)s running.. PID=%(pid)d Port=%(scgi_port)s") % (locals())
    else:
        print _("Server %(version)s running.. PID=%(pid)d Socket=%(scgi_port)s") % (locals())

    # Read configuration file
    CTK.cfg.file = cfg_file
    CTK.cfg.load()

    # Update config tree if required
    config_version.config_version_update_cfg (CTK.cfg)

    # Init CTK
    if scgi_port.isdigit():
        # Ensure that 'localhost' can be resolved
        try:
            socket.getaddrinfo ("localhost", int(scgi_port))
        except socket.gaierror:
            print >> sys.stderr, "[FATAL ERROR]: The 'localhost' host name cannot be resolved.\n"
            sys.exit(1)

        CTK.init (host="localhost", port=int(scgi_port), sec_cookie=True, sec_submit=True)

    else:
        # Remove the unix socket if it already exists
        try:
            mode = os.stat (scgi_port)[stat.ST_MODE]
            if stat.S_ISSOCK(mode):
                print "Removing an old '%s' unix socket.." %(scgi_port)
                os.unlink (scgi_port)
        except OSError:
            pass

        CTK.init (unix_socket=scgi_port, sec_cookie=True, sec_submit=True)

    # At this moment, CTK must be forced to work as a syncronous
    # server. All the initial tests (config file readable, correct
    # installation, etc) must be performed in the safest possible way,
    # ensuring that race conditions don't cause trouble. It will be
    # upgraded to asyncronous mode just before the mainloop is reached
    CTK.set_synchronous (True)

def debug_set_up():
    def debug_callback (sig, frame):
        import code, traceback

        d = {'_frame':frame}       # Allow access to frame object.
        d.update(frame.f_globals)  # Unless shadowed by global
        d.update(frame.f_locals)

        i = code.InteractiveConsole(d)
        message  = "Signal recieved : entering python shell.\nTraceback:\n"
        message += ''.join(traceback.format_stack(frame))
        i.interact(message)

    def trace_callback (sig, stack):
        import traceback, threading

        id2name = dict([(th.ident, th.name) for th in threading.enumerate()])
        for threadId, stack in sys._current_frames().items():
            print '\n# Thread: %s(%d)' %(id2name[threadId], threadId)
            for filename, lineno, name, line in traceback.extract_stack(stack):
                print 'File: "%s", line %d, in %s' %(filename, lineno, name)
                if line:
                    print '  %s' % (line.strip())

    print "DEBUG: SIGUSR1 invokes the console.."
    print "       SIGUSR2 prints a backtrace.."
    signal.signal (signal.SIGUSR1, debug_callback)
    signal.signal (signal.SIGUSR2, trace_callback)

if __name__ == "__main__":
    # Read the arguments
    try:
        scgi_port = sys.argv[1]
        cfg_file  = sys.argv[2]
    except:
        print _("Incorrect parameters: PORT CONFIG_FILE")
        raise SystemExit

    # Debugging mode
    if '-x' in sys.argv:
        debug_set_up()

    # Init
    init (scgi_port, cfg_file)

    # Ancient config file
    def are_vsrvs_num():
        tmp = [True] + [x.isdigit() for x in CTK.cfg.keys('vserver')]
        return reduce (lambda x,y: x and y, tmp)

    if not are_vsrvs_num():
        CTK.publish (r'', PageError.AncientConfig, file=cfg_file)

        while not are_vsrvs_num():
            CTK.step()

        CTK.unpublish (r'')

    # Check config file and set up
    if os.path.exists (cfg_file) and os.path.isdir (cfg_file):
        CTK.publish (r'', PageError.NotWritable, file=cfg_file)

        while os.path.isdir (cfg_file):
            CTK.step()

        CTK.unpublish (r'')

    if not os.path.exists (cfg_file):
        import PageNewConfig
        CTK.publish (r'', PageNewConfig.Render)

        while not os.path.exists (cfg_file):
            CTK.step()

        CTK.unpublish (r'')
        CTK.cfg.file = cfg_file
        CTK.cfg.load()

    if not os.access (cfg_file, os.W_OK):
        CTK.publish (r'', PageError.NotWritable, file=cfg_file)

        while not os.access (cfg_file, os.W_OK):
            CTK.step()

        CTK.unpublish (r'')

    for path in (CHEROKEE_WORKER, CHEROKEE_SERVER, CHEROKEE_ICONSDIR):
        if not os.path.exists (path):
            CTK.publish (r'', PageError.ResourceMissing, path=CHEROKEE_WORKER)

            while not os.path.exists (path):
                CTK.step()

            CTK.unpublish (r'')

    # Set up the error page
    import PageException
    CTK.error.page = PageException.Page

    # Launch the SystemStats ASAP
    import SystemStats
    SystemStats.get_system_stats()

    # Import the Pages
    import PageIndex
    import PageGeneral
    import PageVServers
    import PageVServer
    import PageRule
    import PageEntry
    import PageSources
    import PageSource
    import PageAdvanced
    import PageNewConfig
    import PageHelp
    import PageStatus

    # Wizards 2.0 (TMP!)
    wizards2_path = os.path.realpath (__file__ + "/../wizards2")
    if os.path.exists (wizards2_path):
        import wizards2

    # Init translation
    if CTK.cfg['admin!lang']:
        PageIndex.language_set (CTK.cfg.get_val('admin!lang'))

    # Let's get asyncronous..
    CTK.set_synchronous (False)

    # Run forever
    CTK.run()

    # Kill lingering processes
    try:
        # Parent
        cherokee_admin_pid = os.getppid()
        os.kill (cherokee_admin_pid, signal.SIGTERM)

        # Itself
        os.killpg (0, signal.SIGTERM)
    except OSError:
        pass
