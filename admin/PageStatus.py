import time

from Page import *
from Table import *
from configured import *
from CherokeeManagement import *

SERVER_RUNNING = """
<div class="dialog-online">
 <form id="run-form" action="/stop" method="post">
  <h2>Server status</h2>
  <p>
   Server is running.
   <div style="float: right;">
    <a class="button" href="#" onclick="this.blur(); $('#run-form').submit(); return false;"><span>Stop</span></a>
   </div>
  </p>
 </form>
 <div class="clearfix"></div>
</div>
"""

SERVER_NOT_RUNNING = """
<div class="dialog-offline">
 <form id="run-form" action="/launch" method="post">
  <h2>Server status</h2>
  <p>
   Server is not running.
   <div style="float: right;">
    <a class="button" href="#" onclick="this.blur(); $('#run-form').submit(); return false;"><span>Launch</span></a>
   </div>
  </p>
 </form>
 <div class="clearfix"></div>
</div>
"""

HELPS = [
    ('config_status', "Status")
]

class PageStatus (PageMenu, FormHelper):
    def __init__ (self, cfg=None):
        PageMenu.__init__ (self, 'status', cfg, HELPS)
        FormHelper.__init__ (self, 'status', cfg)

    def _op_render (self):
        self.AddMacroContent ('title', 'Welcome to Cherokee Admin')
        self.AddMacroContent ('content', self.Read('status.template'))

        manager = cherokee_management_get (self._cfg)
        if manager.is_alive():
            self.AddMacroContent ('status', SERVER_RUNNING)
        else:
            self.AddMacroContent ('status', SERVER_NOT_RUNNING)

        extra_info = self._render_extra_info()
        self.AddMacroContent ('extra_info', extra_info)

        return Page.Render(self)

    def _op_handler (self, uri, post):
        return '/'
    
    def _render_extra_info (self):
        txt = ""

        # Server
        table = Table(2, title_left=1, header_style='width="200px"')
        table += ("Server version", VERSION)
        table += ("Server prefix",  PREFIX)
        table += ("Server default path",  WWWROOT)

        manager = cherokee_management_get (self._cfg)
        if manager.is_alive():
            current_pid = str(manager._pid)
        else:
            current_pid = "Not running"

        table += ("Server PID",  current_pid)

        txt += '<h3>Server</h3>'
        txt += self.Indent(table)

        # Configuraion
        table = Table(2, title_left=1, header_style='width="200px"')

        file = self._cfg.file
        if file:
            info = os.stat(file)
            file_status = time.ctime(info.st_ctime)
            table += ("Configuration file", file)
            table += ("Configuration modified", file_status)
        else:
            table += ("Configuration file", 'Not Found')

        txt += '<h3>Configuration</h3>'
        txt += self.Indent(table)

        return txt
