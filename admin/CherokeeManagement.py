import os
import sys
import time
import signal

from configured import *

DEFAULT_DELAY = 2
DEFAULT_PID_LOCATIONS = [
    '/var/run/cherokee.pid',
    os.path.join (PREFIX, 'var/run/cherokee.pid')
]


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
        self._pid = self._get_pid()

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
        pid = os.fork()
        if pid == 0:
            os.setsid()
            os.execl (CHEROKEE_SRV_PATH)
        elif pid > 0:
            time.sleep (DEFAULT_DELAY)
            
    def stop (self):
        if not self._pid:
            return
        os.kill (self._pid, signal.SIGQUIT)
        try:
            os.waitpid (self._pid, 0)
        except:
            pass

    # Protected
    #

    def _get_pid (self):
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
        os.kill (self._pid, signal.SIGHUP)


def is_PID_alive (pid, filter='cherokee'):
    if sys.platform == 'win32':
        raise 'TODO'
    else:
        for line in os.popen('/bin/ps -e').readlines():
            if not filter in line:
                continue

            pid_alive = int(line.strip().split(' ')[0])
            if pid == pid_alive:
                return True
