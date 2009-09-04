import time

from Page import *
from Table import *
from configured import *
from CherokeeManagement import *
from GraphManager import *

# For gettext
N_ = lambda x: x

SERVER_RUNNING = """
<div id="statusarea" class="running">
<form id="run-form" action="/stop" method="post">
 <div id="statustitle">{{_lserver_status}}:</div>
 <div id="statusmsg">{{_server_running}}</div>
 <div id="statusaction"><a href="#" onclick="this.blur(); $('#run-form').submit(); return false;">{{_button_stop}}</a></div>
</form>
</div>
"""

SERVER_NOT_RUNNING = """
<div id="statusarea" class="notrunning">
<form id="run-form" action="/launch" method="post">
 <div id="statustitle">{{_lserver_status}}:</div>
 <div id="statusmsg">{{_server_running}}</div>
 <div id="statusaction"><a href="#" onclick="this.blur(); $('#run-form').submit(); return false;">{{_button_launch}}</a></div>
</form>
</div>
"""

MENU_LANGUAGES = """
<div id="change_language">
  <form name="flanguages" id="flanguages" method="post" action="/change_language">
  <div class="label_lang">{{_llanguages}}</div>
  <div>{{languages_select}}</div>
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

        if graphs_are_active(self._cfg):
            server_graphs = self._render_server_graphs()
        else:
            server_graphs = ''
        self.AddMacroContent ('server_graphs', server_graphs)

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
        txt += '<table width="100%" class="rulestable">'

        # Document root
        #
        tmp = [int(x) for x in self._cfg.keys('vserver')]
        tmp.sort()

        www_root = None
        if tmp:
            www_root = self._cfg.get_val ('vserver!%d!document_root'%(tmp[0]))
        if not www_root:
            www_root = WWWROOT

        # Server
        txt += '<tr><td class="infolab">%s:</td><td>%s</td></tr>' % (_("Version"), VERSION)
        txt += '<tr><td class="infolab">%s:</td><td>%s</td></tr>' % (_("Default WWW"), www_root)
        txt += '<tr><td class="infolab">%s:</td><td>%s</td></tr>' % (_("Prefix"), PREFIX)

        manager = cherokee_management_get (self._cfg)
        if manager.is_alive():
            current_pid = str(manager._pid)
        else:
            current_pid = _("Not running")

        txt += '<tr><td class="infolab">%s:</td><td>%s</td></tr>'  % (_("PID file"), current_pid)

        # Configuration
        file = self._cfg.file
        if file:
            info = os.stat(file)
            file_status = time.ctime(info.st_ctime)
            txt += '<tr><td class="infolab">%s:</td><td>%s</td></tr>'  % (_("Configuration File"), file)
            txt += '<tr><td class="infolab">%s:</td><td>%s</td></tr>'  % (_("Modified"), file_status)
        else:
            txt += '<tr><td class="infolab">%s:</td><td>%s</td></tr>'  % (_("Configuration File"), _('Not Found'))

        txt += '</table>'
        return txt


    def _render_server_graphs (self):
        txt = '<script type="text/javascript" src="/static/js/graphs.js"></script>';
        txt += '<div id="grapharea">'

        txt += '<div id="gmenu">'
        txt += '<ul>'
        txt += '<li id="gmtop"><span>%s</span></li>' % (_("Server Traffic"))
        txt += '<li class="item"><a onclick="graphChangeType(\'traffic\', this.innerHTML)">%s</a></li>'  % (_("Server Traffic"))
        txt += '<li class="item"><a onclick="graphChangeType(\'accepts\', this.innerHTML)">%s</a></li>'  % (_("Accepted Connections"))
        txt += '<li class="item last"><a onclick="graphChangeType(\'timeouts\', this.innerHTML)">%s</a></li>'  % (_("Connections Timeout"))
        txt += '</ul>'
        txt += '</div>'

        txt += '<div id="gtype">'
        txt += '<div id="g1m" class="gbutton"><a onclick="graphChangeInterval(\'1m\')">%s</a></div>' % (_("1 month"))
        txt += '<div id="g1w" class="gbutton"><a onclick="graphChangeInterval(\'1w\')">%s</a></div>' % (_("1 week"))
        txt += '<div id="g1d" class="gbutton"><a onclick="graphChangeInterval(\'1d\')">%s</a></div>' % (_("1 day"))
        txt += '<div id="g6h" class="gbutton"><a onclick="graphChangeInterval(\'6h\')">%s</a></div>' % (_("6 hours"))
        txt += '<div id="g1h" class="gbutton gsel"><a onclick="graphChangeInterval(\'1h\')">%s</a></div>' % (_("1 hour"))
        txt += '</div>'
 
        txt += '<div id="graphdiv">'
        txt += '<img id="graphimg" src="/graphs/server_traffic_1h.png" alt="Graph" />'
        txt += '</div>'
        txt += '</div>'

        txt += """<script type="text/javascript">
                   $(document).ready(function() { refreshGraph(); });
                  </script>
               """

        return txt
