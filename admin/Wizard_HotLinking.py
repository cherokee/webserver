import validations

from config import *
from util import *
from Page import *
from Wizard import *

NOTE_DOMAIN        = "Domain allowed to access the files. Eg: example.com"
NOTE_REDIR         = "Path to the public to use. Eg: /images/forbidden.jpg"
NOTE_TYPE          = "How to handle hot-linking requests."
ERROR_EMPTY_DOMAIN = "A domain name is required"

DATA_VALIDATION = [
    ("tmp!wizard_hotlink!redirection", validations.is_path)
]

CONFIG_RULES = """
%(rule_pre)s!match = and
%(rule_pre)s!match!left = extensions
%(rule_pre)s!match!left!extensions = jpg,jpeg,gif,png,flv
%(rule_pre)s!match!right = not
%(rule_pre)s!match!right!right = header
%(rule_pre)s!match!right!right!header = Referer
%(rule_pre)s!match!right!right!match = ^($|https?://(.*?\.)?%(domain)s)
"""

REPLY_FORBIDDEN = """
%(rule_pre)s!handler = custom_error
%(rule_pre)s!handler!error = 403
"""

REPLY_REDIR = """
%(rule_pre)s!handler = redir
%(rule_pre)s!handler!rewrite!1!show = 0
%(rule_pre)s!handler!rewrite!1!substring = %(redirection)s
"""

TYPES = [
    ('error', 'Show error'),
    ('redir', 'Redirect')
]

class Wizard_Rules_HotLinking (WizardPage):
    ICON = "hotlinking.png"
    DESC = "Stop other domains from hot-linking your media files."
    
    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre, 
                             submit = '/vserver/%s/wizard/HotLinking'%(pre.split('!')[1]),
                             id     = "HotLinking_Page1",
                             title  = _("Hot Linking Prevention Wizard"),
                             group  = WIZARD_GROUP_TASKS)

    def show (self):
        return True

    def _render_content (self, url_pre):
        nick = self._cfg.get_val("%s!nick"%(self._pre))
        if not '.' in nick:
            nick = "example.com"

        txt = '<h1>%s</h1>' % (self.title)
        txt += '<h2>Local Host Name</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('Domain Name'), 'tmp!wizard_hotlink!domain', NOTE_DOMAIN, value=nick)
        txt += self.Indent(table)

        txt += '<h2>Behaviour</h2>'
        table = TableProps()
        self.AddPropOptions_Reload (table, _('Reply type'), 'tmp!wizard_hotlink!type', TYPES, NOTE_TYPE)

        tipe = self._cfg.get_val ('tmp!wizard_hotlink!type')
        if tipe == 'redir':
            self.AddPropEntry (table, _('Redirection'), 'tmp!wizard_hotlink!redirection', NOTE_REDIR)

        txt += self.Indent(table)            

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        tipe = post.pop('tmp!wizard_hotlink!type')

        # Validate
        if tipe == 'redir':
            self.ValidateChange_SingleKey ('tmp!wizard_hotlink!redirection', post, DATA_VALIDATION)
        self.Validate_NotEmpty (post, 'tmp!wizard_hotlink!domain', ERROR_EMPTY_DOMAIN)
        if self.has_errors(): return

        # Incoming info
        domain      = post.pop('tmp!wizard_hotlink!domain')
        redirection = post.pop('tmp!wizard_hotlink!redirection')

        domain = domain.replace ('.', "\\.")

        # Locals
        _, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)

        # Add the new rules
        if tipe == 'redir' and redirection:
            config = (CONFIG_RULES + REPLY_REDIR) % (locals())
        else:
            config = (CONFIG_RULES + REPLY_FORBIDDEN) % (locals())

        self._apply_cfg_chunk (config)

