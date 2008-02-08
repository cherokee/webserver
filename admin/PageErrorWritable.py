from Page import *

class PageErrorWritable (Page):
    def __init__ (self, cfg):
        Page.__init__ (self, 'error_writable', cfg)

    def _op_render (self):
        self.AddMacroContent ('body', PAGE_MENU_LAYOUT)
        self.AddMacroContent ('menu', '')
        self.AddMacroContent ('title', 'ERROR: Configuration file cannot be modified')
        self.AddMacroContent ('content', self.Read('error_writable.template'))
        self.AddMacroContent ('cherokee_conf', self._cfg.file)
        return Page.Render(self)

    def _op_handler (self, uri, post):
        return '/'
