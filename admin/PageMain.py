from Page import *
from CherokeeManagement import *

SERVER_RUNNING = """
Server is running <br />

<form action="/stop" method="post">
 <input type="submit" value="Stop" />
</form>
"""

SERVER_NOT_RUNNING = """
Server is not running <br />

<form action="/launch" method="post">
 <input type="submit" value="Launch" />
</form>
"""

class PageMain (PageMenu):
    def __init__ (self, cfg=None):
        PageMenu.__init__ (self, 'main', cfg)

        self.manager = get_cherokee_management (cfg)

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
    
