import os
import signal

from Page import *

class PageApply (PageMenu):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, cfg)
        self._id = 'apply'

    def _op_render (self):
        self._cfg.save()
        self._restart()
        return '/'

    def _op_handler (self, uri, post):
        return '/'

    def _restart (self):
        pid_cfg = self._cfg["server!pid_file"]
        if not pid_cfg:
            print ("No PID file")
            return True

        pid_file = pid_cfg.value
        if not pid_file:
            return True
        
        try:
            f = open (pid_file, "r")
        except IOError:
            print ("Couldn't read PID file")
            return True
        
        pid = int(f.readline())
        os.kill (pid, signal.SIGHUP)
