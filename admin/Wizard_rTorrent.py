import validations
from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

NOTE_CONNECTION = N_("Where rTorrent XMLRPC is available. The host:port pair, or the Unix socket path.")
NOTE_WEB_DIR    = N_("Web directory where you want rTorrent XMLRPC to be accessible. (Example: /RPC2)")

CONFIG_DIR = """
%(rule_pre_1)s!match = request
%(rule_pre_1)s!match!request = ^%(web_dir)s
%(rule_pre_1)s!handler = scgi
%(rule_pre_1)s!handler!balancer = round_robin
%(rule_pre_1)s!handler!balancer!source!1 = %(src_num)d
"""

CONFIG_SOURCE = """
%(source)s!nick = rTorrent XMLRPC
%(source)s!type = host
%(source)s!host = %(connection)s
"""


class Wizard_Rules_rTorrent (WizardPage):
    ICON = "rtorrent.png"
    DESC = "Configures rTorrent XMLRPC."

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/rTorrent'%(pre.split('!')[1]),
                             id     = "rTorrent_Page1",
                             title  = _("rTorrent Wizard"),
                             group  = _(WIZARD_GROUP_MISC))

    def show (self):
        return True

    def _render_content (self, url_pre):
        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'),   'tmp!wizard_rTorrent!web_dir', _(NOTE_WEB_DIR), value="/RPC2")
        self.AddPropEntry (table, _('Connection'),'tmp!wizard_rTorrent!connection', _(NOTE_CONNECTION), value="localhost:5000")

        txt  = '<h1>%s</h1>' % (self.title)
        txt += self.Indent(table)
        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_rTorrent!connection', _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_rTorrent!web_dir', _(ERROR_EMPTY))
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        connection = post.pop('tmp!wizard_rTorrent!connection')
        web_dir    = post.pop('tmp!wizard_rTorrent!web_dir')

        rule = cfg_vsrv_rule_find_regexp (self._cfg, self._pre, '^'+web_dir)
        if rule:
            return self.report_error (_("Already configured:") + web_dir)

        # Add source
        source = cfg_source_find_interpreter (self._cfg, None, 'rTorrent XMLRPC')
        if not source:
            x, source = cfg_source_get_next (self._cfg)
            config_source = CONFIG_SOURCE % (locals())
            self._apply_cfg_chunk (config_source)

        src_num = int(source.split('!')[-1])

        rule_n, x = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        if not rule_n:
            return self.report_error (_("Couldn't add a new rule."))

        rule_pre_1 = '%s!rule!%d' % (self._pre, rule_n)

        # Add the new rules
        config_dir = CONFIG_DIR % (locals())

        self._apply_cfg_chunk (config_dir)
