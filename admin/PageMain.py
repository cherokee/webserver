from Page import *

class PageMain (PageMenu):
    def __init__ (self):
        PageMenu.__init__ (self, 'main', cfg=None)

    def _op_render (self):
        self.AddMacroContent ('title', 'Welcome to Cherokee Admin')
        self.AddMacroContent ('content', self.Read('main.template'))

        return Page.Render(self)

    def _op_handler (self, uri, post):
        return '/'
    
