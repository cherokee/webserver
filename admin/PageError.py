from Page import *

class PareError_LaunchFail (Page):
    def __init__ (self, cfg, error):
        Page.__init__ (self, 'error_couldnt_launch', cfg)

        self._cfg         = cfg
        self._error_raw   = error
        self._error_lines = error.split("\n")

        for line in self._error_lines:
            if not line:
                continue

            if line.startswith("{'type': "):
                src = "self._error = " + self._error_lines[0]
                exec(src)
                break

    def _op_render (self):
        template = 'error_couldnt_launch.template'

        self.AddMacroContent ('menu', '')
        self.AddMacroContent ('help', '')
        self.AddMacroContent ('body', PAGE_BASIC_LAYOUT_NOBAR)
        self.AddMacroContent ('title', 'ERROR: %s'%(self._error['title']))
        self.AddMacroContent ('content', self.Read(template))
        self.AddMacroContent ('icons_dir', CHEROKEE_ICONSDIR)

        self.AddMacroContent ('time',        self._error.get('time', ''))
        self.AddMacroContent ('admin_url',   self._error.get('admin_url', ''))
        self.AddMacroContent ('backtrace',   self._error.get('backtrace', ''))
        self.AddMacroContent ('description', self._error.get('description', ''))
        self.AddMacroContent ('title',       self._error.get('title', self._error_raw))
        
        return Page.Render(self)

    def _op_handler (self, uri, post):
        return '/'


class PageError (Page):
    CONFIG_NOT_WRITABLE = 'not_writable'
    ICONS_DIR_MISSING   = 'icons_dir_missing'

    def __init__ (self, cfg, tipe, **kwargs):
        Page.__init__ (self, 'error_%s'%(tipe), cfg)
        self.type   = tipe
        self._error = None

        for arg in kwargs:
            self.AddMacroContent (arg, kwargs[arg])

    def _op_render (self):
        template = 'error_%s.template' % (self.type)

        self.AddMacroContent ('menu', '')
        self.AddMacroContent ('help', '')
        self.AddMacroContent ('body', PAGE_BASIC_LAYOUT_NOBAR)
        self.AddMacroContent ('title', 'ERROR: %s'%(ERRORS_TITLE[self.type]))
        self.AddMacroContent ('content', self.Read(template))
        self.AddMacroContent ('cherokee_conf', self._cfg.file)
        self.AddMacroContent ('icons_dir', CHEROKEE_ICONSDIR)
        return Page.Render(self)

    def _op_handler (self, uri, post):
        return '/'


# For gettext
N_ = lambda x: x

ERRORS_TITLE = {
    PageError.CONFIG_NOT_WRITABLE : N_('Configuration file cannot be modified'),
    PageError.ICONS_DIR_MISSING:    N_('Icons directory is missing'),
}
