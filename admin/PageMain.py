from Page import *

class PageMain (PageMenu):
    def __init__ (self, cfg=None):
        PageMenu.__init__ (self, 'main', cfg)

    def _op_render (self):
        self.AddMacroContent ('title', 'Welcome to Cherokee Admin')
        self.AddMacroContent ('content', self.Read('main.template'))

        return Page.Render(self)

    def _op_handler (self, uri, post):
        return '/'
    
