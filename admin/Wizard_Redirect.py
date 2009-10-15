import string
import validations
from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

NOTE_SRC_HOST = N_("This domain name will be used as nickname for the new virtual server. More domains names can be added to the virtual server later on.")
NOTE_TRG_HOST = N_("Domain name to which requests will be redirected.")
ERROR_EMPTY   = N_("This field can not be empty.")

CONFIG_VSRV = """
%(pre_vsrv)s!nick = %(host_src)s
%(pre_vsrv)s!document_root = /dev/null

%(pre_vsrv)s!rule!1!match = default
%(pre_vsrv)s!rule!1!handler = redir
%(pre_vsrv)s!rule!1!handler!rewrite!1!show = 1
%(pre_vsrv)s!rule!1!handler!rewrite!1!regex = /(.*)$
%(pre_vsrv)s!rule!1!handler!rewrite!1!substring = http://%(host_trg)s/$1
"""

EXTRA_WILDCARD = """
%(pre_vsrv)s!match = wildcard
%(pre_vsrv)s!match!domain!1 = %(host_src)s
"""

EXTRA_REHOST = """
%(pre_vsrv)s!match = rehost
%(pre_vsrv)s!match!regex!1 = %(host_src)s
"""


DATA_VALIDATION = [
    ("tmp!wizard_redir!host_src", (validations.is_new_host, 'cfg'))
]

class Wizard_VServer_Redirect (WizardPage):
    ICON = "redirect.jpg"
    DESC = _("New virtual server redirecting every request to another domain host.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/Redirect',
                             id     = "Redirect_Page1",
                             title  = _("Redirection Wizard"),
                             group  = _(WIZARD_GROUP_TASKS))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        table = TableProps()
        self.AddPropEntry (table, _('Origin Domain'), 'tmp!wizard_redir!host_src', _(NOTE_SRC_HOST))
        self.AddPropEntry (table, _('Target Domain'), 'tmp!wizard_redir!host_trg', _(NOTE_TRG_HOST), value="www.example.com")
        txt += self.Indent(table)

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_redir!host_src', _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_redir!host_trg', _(ERROR_EMPTY))
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        host_src = post.pop('tmp!wizard_redir!host_src')
        host_trg = post.pop('tmp!wizard_redir!host_trg')

        # Locals
        pre_vsrv = cfg_vsrv_get_next (self._cfg)

        # Add the new rules
        config = CONFIG_VSRV % (locals())

        # Analise the source domain
        wildcard_domain = string.letters + "_-.*?"
        is_wildcard = reduce (lambda x,y: x and y, [c in wildcard_domain for c in host_src])

        if is_wildcard:
            config += EXTRA_WILDCARD % (locals())
        else:
            config += EXTRA_REHOST % (locals())

        # Apply it
        self._apply_cfg_chunk (config)
