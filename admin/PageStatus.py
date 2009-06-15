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
  <h2>{{_lserver_status}}</h2>
  <p>
   {{_server_running}}
   <div style="float: right;">
    <a class="button" href="#" onclick="this.blur(); $('#run-form').submit(); return false;"><span>{{_button_stop}}</span></a>
   </div>
  </p>
 </form>
</div>
"""

SERVER_NOT_RUNNING = """
<div class="dialog-offline">
 <form id="run-form" action="/launch" method="post">
  <h2>{{_lserver_status}}</h2>
  <p>
   {{_server_running}}
   <div style="float: right;">
    <a class="button" href="#" onclick="this.blur(); $('#run-form').submit(); return false;"><span>{{_button_launch}}</span></a>
   </div>
  </p>
 </form>
</div>
"""

MENU_LANGUAGES = """
<div id="change_language">
  <div class="label_lang">{{_llanguages}}</div>
  <form name="flanguages" id="flanguages" method="post" action="/change_language">
    {{languages_select}}
  </form>
</div>
"""

BETA_TESTER_NOTICE = N_("""
<h3>Beta testing</h3>
<p>Individuals like yourself who download and test the latest developer snapshots of Cherokee Web Server help us to create the highest quality product.</p>
<p>For that, we thank you.</p>
""")

FEEDBACK_NOTICE = N_("""
If you wanted to let us know something about Cherokee: what you
like the most or what you would improve, please do not hesitate to
<a href="/feedback">drop us a line</a>.
""")

PROUD_USERS_NOTICE = N_("""
We would love to know that you are using Cherokee. Submit your domain
name and  it will be <a href="http://www.cherokee-project.com/cherokee-domain-list.html">
listed on the Cherokee Project web site</a>.
""")

HELPS = [
    ('config_status', N_("Status"))
]

class PageStatus (PageMenu, FormHelper):
    def __init__ (self, cfg=None):
        PageMenu.__init__ (self, 'status', cfg, HELPS)
        FormHelper.__init__ (self, 'status', cfg)

    def _op_render (self):
        self.AddMacroContent ('title', _('Welcome to Cherokee Admin'))
        self.AddMacroContent ('_description', _('This interface will assist in configuring the Cherokee Web Server.'))
        self.AddMacroContent ('content', self.Read('status.template'))

        if 'b' in VERSION:
            notice = self.Dialog(_(BETA_TESTER_NOTICE))
            self.AddMacroContent ('beta_tester', notice)
        else:
            self.AddMacroContent ('beta_tester', '')

        manager = cherokee_management_get (self._cfg)
        self.AddMacroContent ('_lserver_status', _('Server status'))
        if manager.is_alive():
            self.AddMacroContent ('_server_running', _('Server is running.'))
            self.AddMacroContent ('_button_stop',    _('Stop'))
            self.AddMacroContent ('status', SERVER_RUNNING)
        else:
            self.AddMacroContent ('_server_running', _('Server is not running.'))
            self.AddMacroContent ('_button_launch',    _('Launch'))
            self.AddMacroContent ('status', SERVER_NOT_RUNNING)

        extra_info = self._render_extra_info()
        self.AddMacroContent ('_linformation', _('Information'))
        self.AddMacroContent ('extra_info', extra_info)

        # Translation
        if len(AVAILABLE_LANGUAGES) > 1:
            self.AddMacroContent ('_llanguages',    _('Language:'))
            self.AddMacroContent ('translation', MENU_LANGUAGES)
            options = [('', _('Choose'))] + AVAILABLE_LANGUAGES
            js = "javascript:$('#flanguages').submit()"
            selbox = EntryOptions ('language', options, onChange=js)
            self.AddMacroContent ('languages_select', str(selbox))
        else:
            self.AddMacroContent ('translation', '')

        self.AddMacroContent ('_lfeedback', _('Feedback'))
        self.AddMacroContent ('_feedback',  _(FEEDBACK_NOTICE))

        self.AddMacroContent ('_lproudusers', _('Proud Cherokee Users'))
        self.AddMacroContent ('_proudusers',  _(PROUD_USERS_NOTICE))

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
