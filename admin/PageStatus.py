import time

from Page import *
from Table import *
from configured import *
from CherokeeManagement import *

# For gettext
N_ = lambda x: x

SERVER_RUNNING = """
<div class="dialog-online">
 <form id="run-form" action="/stop" method="post">
  <h2>%s</h2>
  <p>
   %s
   <div style="float: right;">
    <a class="button" href="#" onclick="this.blur(); $('#run-form').submit(); return false;"><span>%s</span></a>
   </div>
  </p>
 </form>
</div>
""" % (N_('Server status'), N_('Server is running.'), N_('Stop'))

SERVER_NOT_RUNNING = """
<div class="dialog-offline">
 <form id="run-form" action="/launch" method="post">
  <h2>%s</h2>
  <p>
   %s
   <div style="float: right;">
    <a class="button" href="#" onclick="this.blur(); $('#run-form').submit(); return false;"><span>%s</span></a>
   </div>
  </p>
 </form>
</div>
""" % (N_('Server status'), N_('Server is not running.'), N_('Launch'))

BETA_TESTER_NOTICE = "<h3>%s</h3>" % (N_('Beta testing')) + \
                     "<p>%s</p>" % (N_('Individuals like yourself who download and test the latest developer snapshots of Cherokee Web Server help us to create the highest quality product.')) + \
                     "<p>%s</p>" % (N_('For that, we thank you.'))

HELPS = [
    ('config_status', N_("Status"))
]

class PageStatus (PageMenu, FormHelper):
    def __init__ (self, cfg=None):
        PageMenu.__init__ (self, 'status', cfg, HELPS)
        FormHelper.__init__ (self, 'status', cfg)

    def _op_render (self):
        self.AddMacroContent ('title', _('Welcome to Cherokee Admin'))
        self.AddMacroContent ('content', self.Read('status.template'))

        if 'b' in VERSION:
            notice = self.Dialog(BETA_TESTER_NOTICE)
            self.AddMacroContent ('beta_tester', notice)
        else:
            self.AddMacroContent ('beta_tester', '')

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
        table = Table(2, title_left=1, header_style='width="130px"')
        table += (_("Version"), VERSION)
        table += (_("Prefix"),  PREFIX)
        table += (_("WWW Root"),  WWWROOT)

        manager = cherokee_management_get (self._cfg)
        if manager.is_alive():
            current_pid = str(manager._pid)
        else:
            current_pid = _("Not running")

        table += (_("PID file"),  current_pid)

        txt += '<h3>%s</h3>' % (_('Server'))
        txt += self.Indent(table)

        # Configuraion
        table = Table(2, title_left=1, header_style='width="130px"')

        file = self._cfg.file
        if file:
            info = os.stat(file)
            file_status = time.ctime(info.st_ctime)
            table += (_("Path"), file)
            table += (_("Modified"), file_status)
        else:
            table += (_("Configuration file"), _('Not Found'))

        txt += '<h3>%s</h3>' % (_('Configuration File'))
        txt += self.Indent(table)

        return txt
