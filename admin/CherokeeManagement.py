import os
import sys
import signal

DEFAULT_PID_LOCATIONS = [
    '/var/run/cherokee.pid',
    '/usr/local/var/run/cherokee.pid',
    '/opt/cherokee/var/run/cherokee.pid'
]


cherokee_management = None
def get_cherokee_management (cfg):
    global cherokee_management

    # Fast path
    if cherokee_management:
        return cherokee_management

    # Needs to create a new object
    cherokee_management = CherokeeManagement(cfg)
    return cherokee_management


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
        print ("TODO: Launch server")

    # Protected
    #

    def _get_pid (self):
        pid_file = None

        # Look up the configuration
        pid_cfg = self._cfg["server!pid_file"]
        if pid_cfg:
            pid_file = pid_cfg.value

        # If there wasn't an entry..
        if not pid_file:
            for file in DEFAULT_PID_LOCATIONS:
                if os.access (file, os.R_OK):
                    pid_file = file
                    break

        if not pid_file:
            print ("Couldn't find a suitable PID file")
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


def is_PID_alive (pid, filter):
    if sys.platform == 'win32':
        raise 'TODO'
    else:
        for line in os.popen('/bin/ps -e').readlines():
            if not filter in line:
                continue

            pid_alive = int(line.strip().split(' ')[0])
            if pid == pid_alive:
                return True
