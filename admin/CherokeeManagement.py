import os
import sys
import time
import signal
import select

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
        sys.stdout.flush()
        sys.stderr.flush()

        errout, errin = os.pipe()        

        try: 
            pid = os.fork() 
            if pid == 0:
                os.dup2(errin, 2)
                try:
                    os.setsid() 
                    os.chdir("/") 
                    os.umask(0) 
                except:
                    pass
                try:
                    os.execl(CHEROKEE_SRV_PATH)
                finally:
                    os._exit(1)
        except OSError, e: 
            print >>sys.stderr, "fork #2 failed: %d (%s)" % (e.errno, e.strerror) 
            sys.exit(1)         

        print "Cherokee PID %d" % pid 

        os.close(errin)
        childerr = os.fdopen(errout, 'r')

        errors = []
        for i in range(5):
            r,w,x = select.select ([errout], [], [], 1)
            print r,w,x
            if errout in r:
                error = childerr.readline().strip()
                if not error: continue
                errors.append (error)

        if not errors:
            self._pid = pid
            return

        self._pid = None
        return '\r'.join (errors)
            
    def stop (self):
        if not self._pid:
            return
        try:
            os.kill (self._pid, signal.SIGQUIT)
            os.waitpid (self._pid, 0)
        except:
            pass

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
