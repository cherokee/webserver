from config import *
from util import *
from Page import *
from Wizard import *

NOTE_SRC_HOST   = _("New host name.")
NOTE_TRG_HOST   = _("Host name to which requests will be redirected.")
ERROR_EMPTY_SRC = _("A source host name is required")
ERROR_EMPTY_TRG = _("A target host name is required")

CONFIG_VSRV = """
%(pre_vsrv)s!nick = %(host_src)s
%(pre_vsrv)s!document_root = /dev/null

%(pre_vsrv)s!rule!1!match = default
%(pre_vsrv)s!rule!1!handler = redir
%(pre_vsrv)s!rule!1!handler!rewrite!1!show = 1
%(pre_vsrv)s!rule!1!handler!rewrite!1!regex = /(.*)$
%(pre_vsrv)s!rule!1!handler!rewrite!1!substring = http://%(host_trg)s/$1
"""

class Wizard_VServer_Redirect (WizardPage):
    ICON = "redirect.jpg"
    DESC = "New virtual server redirecting every request to another domain host."
    
    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre, 
                             submit = '/vserver/wizard/Redirect',
                             id     = "Redirect_Page1",
                             title  = _("Redirection Wizard"),
                             group  = WIZARD_GROUP_TASKS)

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'),    'tmp!wizard_redir!host_src', NOTE_SRC_HOST)
        self.AddPropEntry (table, _('Target Host Name'), 'tmp!wizard_redir!host_trg', NOTE_TRG_HOST, value="www.example.com")
        txt += self.Indent(table)

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Validate
        self.Validate_NotEmpty (post, 'tmp!wizard_redir!host_src', ERROR_EMPTY_SRC)
        self.Validate_NotEmpty (post, 'tmp!wizard_redir!host_trg', ERROR_EMPTY_TRG)
        if self.has_errors(): return

        # Incoming info
        host_src = post.pop('tmp!wizard_redir!host_src')
        host_trg = post.pop('tmp!wizard_redir!host_trg')

        # Locals
        pre_vsrv = cfg_vsrv_get_next (self._cfg)
        
        # Add the new rules
        config = CONFIG_VSRV % (locals())
        self._apply_cfg_chunk (config)
