import os
import sys
import time
import threading

from Page import *
from Table import *
from GraphManager import *


OPEN_ERROR_SLEEP = 5          # secs
INITIAL_TAIL_POS = 20 * 1024  # bytes

SINGLE_TIME_HTML = """
"""

LOG_ENTRY_HTML = """
<h1>%(title)s</h1>
<p>
  <div id="log_vsrv_%(vserver)s"></div><br/>
  <i>%(log_file)s</i>
</p>


<script type="text/javascript">
  function log_%(vserver)s_reload() {
    $("#log_vsrv_%(vserver)s").load("/monitor/content/%(vserver)s");
  }

  window.setInterval (function(){ 
       log_%(vserver)s_reload();
  }, 1000 );

  log_%(vserver)s_reload();
</script>
"""


class LogViewerWidget (threading.Thread):
    def __init__ (self, filename):
        # Create the thread
        threading.Thread.__init__ (self)
        self.setDaemon (1)
        
        # File
        self.file     = None
        self.filename = filename

        # Render
        self.render   = ""

    def __open_file (self):
        # Already opened
        if self.file != None:
            return False

        # Open
        try:
            self.file = open (self.filename, 'r')
        except IOError:
            return True
        
        # Seek
        try:
            filesize = os.path.getsize (self.filename)
        except OSError:
            return True

        if filesize > INITIAL_TAIL_POS:
            self.file.seek (filesize - INITIAL_TAIL_POS)
            self.file.readline()

        return False


    def render_line (self, line):
        assert False, "Pure Virtual Method"


    def run (self):
        while True:
            # Ensure the file is open
            error = self.__open_file()
            if error:
                time.sleep (OPEN_ERROR_SLEEP)
                continue

            # Read
            where = self.file.tell()
            line = self.file.readline()
            if line:
                self.render_line (line)
            else:
                time.sleep(1)
                self.file.seek (where)


class Cherokee_ErrorLog_Widget (LogViewerWidget):
    def __init__ (self, filename):
        LogViewerWidget.__init__ (self, filename)

    def render_line (self, line):
        if line.startswith("{'type': \""):
            exec ("error = %s" %(line))
            self._render_parsed (error)
        else:
            self._render_unknown (line)

    def _render_parsed (self, error):
        entry = "<p>(%s) <b>%s</b>: %s<br/>%s</p>" %(error['time'], error['title'], error['type'], error['description'])
        print entry
        self.render = entry + self.render

    def _render_unknown (self, line):
        self.render = "%s<br/>" %(line) + self.render


# Globals
widgets_init   = False
widget_objs    = []
log_to_widget  = {}
vsrv_to_widget = {}


def init_log_readers (cfg):
    global widget_objs
    global log_to_widget
    global vsrv_to_widget

    # Search the vservers
    tmp = cfg.keys('vserver')
    tmp.sort(lambda x,y: cmp(int(x), int(y)))

    for v in tmp:
        pre = 'vserver!%s' % (v)
        if not cfg.get_val ('%s!logger'%(pre)):
            continue

        path = cfg.get_val ('%s!logger!error!filename' %(pre))
        if path and not log_to_widget.has_key(path):
            widget = Cherokee_ErrorLog_Widget (path)

            widget_objs.append (widget)
            log_to_widget[path] = widget
            vsrv_to_widget[v]   = widget

    # Launch them
    for reader in widget_objs:
        reader.start()
    

class PageMonitor (PageMenu, FormHelper):
    def __init__ (self, cfg=None):
        PageMenu.__init__ (self, 'monitor', cfg)
        FormHelper.__init__ (self, 'monitor', cfg)

        global widgets_init
        if not widgets_init:
            init_log_readers (self._cfg)
            widgets_init = True

    def _op_handler (self, uri, post):
        if "/content/" in uri:
            vsrv   = uri[uri.index("/content/"):].split('/')[2]
            if not vsrv_to_widget.has_key(vsrv):
                return "/"

            widget = vsrv_to_widget[vsrv]
            if len(widget.render) > 0:
                render = widget.render
            else:
                render = "Empty"

            return render
        else:
            self._op_render()
    
    def _op_render (self):
        content = SINGLE_TIME_HTML

        for vsrv in vsrv_to_widget:
            vsrv_nick = self._cfg.get_val('vserver!%s!nick'%(vsrv))

            # Log reader
            widget = vsrv_to_widget[vsrv]
            entry = LOG_ENTRY_HTML %(
                {'title' :    "Error log of '%s'" %(vsrv_nick),
                 'log_file':  widget.filename,
                 'vserver':   vsrv})
            content += entry
            
        self.AddMacroContent ('title',   _('Cherokee Web Server Monitor'))
        self.AddMacroContent ('content', content)
        return Page.Render(self)
