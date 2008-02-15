from Page import *

class PageError (Page):
    CONFIG_NOT_FOUND    = 'not_found'
    CONFIG_NOT_WRITABLE = 'not_writable'
    ICONS_DIR_MISSING   = 'icons_dir_missing'

    def __init__ (self, cfg, tipe):
        Page.__init__ (self, 'error_%s'%(tipe), cfg)
        self.type = tipe

    def _op_render (self):
        self.AddMacroContent ('menu', '')
        self.AddMacroContent ('help', '')
        self.AddMacroContent ('body', PAGE_BASIC_LAYOUT)
        self.AddMacroContent ('title', 'ERROR: %s'%(ERRORS_TITLE[self.type]))
        self.AddMacroContent ('content', self.Read('error_%s.template'%(self.type)))
        self.AddMacroContent ('cherokee_conf', self._cfg.file)
        self.AddMacroContent ('icons_dir', CHEROKEE_ICONSDIR)
        return Page.Render(self)

    def _op_handler (self, uri, post):
        return '/'


ERRORS_TITLE = {
    PageError.CONFIG_NOT_FOUND :    'Configuration file not found',
    PageError.CONFIG_NOT_WRITABLE : 'Configuration file cannot be modified',
    PageError.ICONS_DIR_MISSING:    'Icons directory is missing'
}
