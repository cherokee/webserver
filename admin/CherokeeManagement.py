import os
import sys
import time
import signal
from select import select
from subprocess import *

from consts import *
from configured import *

DEFAULT_DELAY = 2
DEFAULT_PID_LOCATIONS = [
    '/var/run/cherokee.pid',
    os.path.join (PREFIX, 'var/run/cherokee.pid')
]

CHEROKEE_MIN_DEFAULT_CONFIG = """# Default configuration
server!pid_file = %s
vserver!default!document_root = /tmp
vserver!default!directory!/!handler = common
vserver!default!directory!/!priority = 1
""" % (DEFAULT_PID_LOCATIONS[0])


# Cherokee Management 'factory':
# 

cherokee_management = None

def cherokee_management_get (cfg):
    global cherokee_management

    # Fast path
    if cherokee_management:
        return cherokee_management

    # Needs to create a new object
    cherokee_management = CherokeeManagement(cfg)
    return cherokee_management

def cherokee_management_reset ():
    global cherokee_management
    cherokee_management = None


# Cherokee Management class
#

class CherokeeManagement:
    def __init__ (self, cfg):
        self._cfg = cfg
        self._pid = self._get_running_pid()

    # Public
    #

    def save (self, restart=True):
        self._cfg.save()
        if restart:
            self._restart()

    def is_alive (self):
        if not self._pid:
            return False
        return is_PID_alive (self._pid)

    def launch (self):
        def daemonize():
            os.setsid() 

        p = Popen ([CHEROKEE_SRV_PATH], stdout=PIPE, stderr=PIPE, preexec_fn=daemonize, close_fds=True)
        stdout_f,  stderr_f  = (p.stdout, p.stderr)
        stdout_fd, stderr_fd = stdout_f.fileno(), stderr_f.fileno()
        stdout,    stderr    = '', ''

        while True:
            r,w,e = select([stdout_fd, stderr_fd], [], [stdout_fd, stderr_fd], 1)

            if e:
                self._pid = None
                return "Could access file descriptors: " + str(e)

            if stdout_fd in r:
                stdout += stdout_f.read(1)
            if stderr_fd in r:
                stderr += stderr_f.read(1)

            if stderr.count('\n'):
                self._pid = None
                return stderr

            if stdout.count('\n') > 1:
                break

        self._pid = p.pid
        time.sleep(2)
        return None

    def stop (self):
        if not self._pid:
            return
        try:
            os.kill (self._pid, signal.SIGQUIT)
            os.waitpid (self._pid, 0)
        except:
            pass

    def create_config (self, file):
        if os.path.exists (file):
            return

        f = open(file, 'w+')
        f.write (CHEROKEE_MIN_DEFAULT_CONFIG)
        f.close()

    # Protected
    #

    def _get_running_pid (self):
        pid_file = None

        # Look up the configuration
        pid_cfg = self._cfg["server!pid_file"]
        if pid_cfg:
            pid_file = pid_cfg.value
            if not os.access (pid_file, os.R_OK):
                pid_file = None

        # If there wasn't an entry..
        if not pid_file:
            for file in DEFAULT_PID_LOCATIONS:
                if os.access (file, os.R_OK):
                    pid_file = file
                    break

        if not pid_file:
            return

        try:
            f = open (pid_file, "r")
        except IOError:
            print ("Couldn't read PID file: %s" % (pid_file))
            return
        
        return int(f.readline())

    def _restart (self):
        if not self._pid:
            return
        try:
            os.kill (self._pid, signal.SIGHUP)
        except:
            pass


def is_PID_alive (pid, filter='cherokee'):
    if sys.platform == 'win32':
        raise 'TODO'
    else:
        f = os.popen('/bin/ps -e')
        lines = f.readlines()
        try:
            f.close()
        except:
            pass

        for line in lines:
            if not filter in line:
                continue

            pid_alive = int(line.strip().split(' ')[0])
            if pid == pid_alive:
                return True
