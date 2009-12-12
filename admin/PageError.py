from Page import *

ERROR_LAUNCH_URL_ADMIN = """
<div class="error-suggestion">The server suggests to check <a href="%s">this page</a>. Most probably the problem can he solved in there.</div>
"""

class PageError_LaunchFail (Page):
    def __init__ (self, cfg, error):
        Page.__init__ (self, 'error_couldnt_launch', cfg)

        self._cfg         = cfg
        self._error       = None
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
        if not self._error:
            return self._op_render_unknown()

        return self._op_render_error()

    def _op_render_error (self):
        template = 'error_couldnt_launch.template'

        self.AddMacroContent ('menu', '')
        self.AddMacroContent ('help', '')
        self.AddMacroContent ('body', PAGE_BASIC_LAYOUT_NOBAR)
        self.AddMacroContent ('title', 'ERROR: %s'%(self._error['title']))
        self.AddMacroContent ('content', self.Read(template))
        self.AddMacroContent ('icons_dir', CHEROKEE_ICONSDIR)

        # admin
        admin_url = self._error.get('admin_url')
        if admin_url:
            admin_url_msg = ERROR_LAUNCH_URL_ADMIN % (admin_url)
        else:
            admin_url_msg = ''

        # debug
        debug = self._error.get('debug')
        if debug:
            debug_msg = '<div class="error-debug">%s</div>' %(debug)
        else:
            debug_msg = ''

        # backtrace
        backtrace = self._error.get('backtrace')
        if backtrace:
            backtrace_msg = '<div class="error-backtrace">%s</div>' %(backtrace)
        else:
            backtrace_msg = ''

        self.AddMacroContent ('time',          self._error.get('time', ''))
        self.AddMacroContent ('debug_msg',     debug_msg)
        self.AddMacroContent ('admin_url_msg', admin_url_msg)
        self.AddMacroContent ('backtrace_msg', backtrace_msg)
        self.AddMacroContent ('description',   self._error.get('description', ''))
        self.AddMacroContent ('title',         self._error.get('title', self._error_raw))

        return Page.Render(self)

    def _op_render_unknown (self):
        template = 'error_couldnt_launch.template'

        self.AddMacroContent ('menu', '')
        self.AddMacroContent ('help', '')
        self.AddMacroContent ('body',       PAGE_BASIC_LAYOUT_NOBAR)
        self.AddMacroContent ('title',      'Unknown Error')
        self.AddMacroContent ('content',    self.Read(template))
        self.AddMacroContent ('icons_dir',  CHEROKEE_ICONSDIR)

        debug_msg = '<div class="error-debug">%s</div>' %(self._error_raw)

        self.AddMacroContent ('time',          '')
        self.AddMacroContent ('backtrace_msg', '')
        self.AddMacroContent ('admin_url_msg', '')
        self.AddMacroContent ('debug_msg',     debug_msg)
        self.AddMacroContent ('description',   'Something unexpected just happened.')

        return Page.Render(self)


    def _op_handler (self, uri, post):
        return '/'


class PageInternelError (Page):
    def __init__ (self, trace):
        Page.__init__ (self, 'error_internal', None)
        self.trace = trace

    def _op_handler (self, uri, post):
        return '/'

    def _op_render (self):
        template = 'error_internal.template'

        self.AddMacroContent ('menu',      '')
        self.AddMacroContent ('help',      '')
        self.AddMacroContent ('body',       PAGE_BASIC_LAYOUT_NOBAR)
        self.AddMacroContent ('title',     'Internal Error')
        self.AddMacroContent ('backtrace',  self.trace)
        self.AddMacroContent ('content',    self.Read(template))
        return Page.Render(self)


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
