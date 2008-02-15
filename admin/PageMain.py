from Page import *
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

class PageMain (PageMenu):
    def __init__ (self, cfg=None):
        PageMenu.__init__ (self, 'main', cfg)

        self.manager = cherokee_management_get (cfg)

    def _op_render (self):
        self.AddMacroContent ('title', 'Welcome to Cherokee Admin')
        self.AddMacroContent ('content', self.Read('main.template'))

        if self.manager.is_alive():
            self.AddMacroContent ('status', SERVER_RUNNING)
        else:
            self.AddMacroContent ('status', SERVER_NOT_RUNNING)

        return Page.Render(self)

    def _op_handler (self, uri, post):
        return '/'
    
