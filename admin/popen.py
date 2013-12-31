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

import os
import sys
import errno
import fcntl
import select
import traceback

POLLING_LAPSE   = 0.2
READ_CHUNK_SIZE = 2**20

try:
    MAXFD = os.sysconf("SC_OPEN_MAX")
except:
    MAXFD = 256


def set_non_blocking (fd):
    fl = fcntl.fcntl (fd, fcntl.F_GETFL)
    fcntl.fcntl (fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)


def popen_sync (command, env=None, stdout=True, stderr=True, retcode=True, cd=None, su=None):
    """This function implements a subset of the functionality provided
    by the subprocess.Popen class. The subprocess module in Python 2.4
    and 2.5 have some problems dealing with processes termination on
    multi-thread environments (593800, 1731717)."""

    stdin_r,  stdin_w  = os.pipe()
    stdout_r, stdout_w = os.pipe()
    stderr_r, stderr_w = os.pipe()

    pid = os.fork()
    if pid == 0:
        # Close parent's pipe ends
        os.close (stdin_w)
        os.close (stdout_r)
        os.close (stderr_r)

        # Dup fds for child
        os.dup2 (stdin_r,  0)
        os.dup2 (stdout_w, 1)
        os.dup2 (stderr_w, 2)

        # Close fds
        for i in xrange(3, MAXFD):
            try:
                os.close (i)
            except:
                pass

        # Change directory
        if cd:
            try:
                os.chdir (cd)
            except Except, e:
                print >> sys.stderr, "Could not change directory to: %s" %(cd)
                print >> sys.stderr, traceback.format_exc()

        # Change user
        if su:
            try:
                os.setuid (su)
            except Except, e:
                print >> sys.stderr, "Could not set user: %s" %(su)
                print >> sys.stderr, traceback.format_exc()

        # Pass control to the executable
        if not env:
            os.execv ('/bin/sh', ['sh', '-c', command])
        else:
            os.execve('/bin/sh', ['sh', '-c', command], env)

    # Poll on child's process outputs
    buf_stderr = ''
    buf_stdout = ''

    set_non_blocking (stdout_r)
    set_non_blocking (stderr_r)

    # Read stdout and stderr
    while True:
        rlist, wlist, xlist = select.select ([stdout_r, stderr_r], [], [stdout_r, stderr_r], POLLING_LAPSE)

        if stderr_r in rlist:
            data = ''
            try:
                data = os.read (stderr_r, READ_CHUNK_SIZE)
            except OSError, e:
                if e[0] in (errno.EAGAIN,):
                    raise
            if data:
                buf_stderr += data

        if stdout_r in rlist:
            data = ''
            try:
                data = os.read (stdout_r, READ_CHUNK_SIZE)
            except OSError, e:
                if e[0] in (errno.EAGAIN,):
                    raise
            if data:
                buf_stdout += data

        # Has it finished?
        try:
            pid_ret, sts = os.waitpid (pid, os.WNOHANG)
            if pid_ret == pid:
                if os.WIFSIGNALED(sts):
                    returncode = -os.WTERMSIG(sts)
                elif os.WIFEXITED(sts):
                    returncode = os.WEXITSTATUS(sts)
                break
        except OSError, e:
            returncode = None
            break

    # Clean up
    os.close (stdin_w)
    os.close (stderr_r)
    os.close (stdout_r)

    os.close (stdin_r)
    os.close (stderr_w)
    os.close (stdout_w)

    # Return information
    ret = {'command': command,
           'stdout':  buf_stdout,
           'stderr':  buf_stderr,
           'retcode': returncode}

    return ret

if __name__ == "__main__":
    print popen_sync ("ls")
    print popen_sync ("lsg")
    print popen_sync ("""cat <<EOF\nThis\nis\na\ntest\nEOF\n""")
