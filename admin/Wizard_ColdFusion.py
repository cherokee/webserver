"""
ColdFussion wizard.

Last update:
* Cherokee 0.99.25
* Adobe ColdFusion 9
"""
import validations
import urllib

from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

NOTE_VSRV_NAME = N_("Name of the new domain that will be created.")
NOTE_SOURCE = N_('The pair IP:port of the server running ColdFusion. You can add more later to have the load balanced.')
NOTE_WEB_DIR = N_("Web folder under which Coldfusion will be accessible.")
ERROR_NO_SRC  = N_("ColdFusion is not running on the specified host.")

SOURCE = """
source!%(src_num)d!env_inherited = 0
source!%(src_num)d!type = host
source!%(src_num)d!nick = ColdFusion %(src_num)d
source!%(src_num)d!host = %(new_source)s
"""

CONFIG_VSRV = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = /dev/null

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = proxy
%(vsrv_pre)s!rule!1!handler!balancer = ip_hash
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
%(vsrv_pre)s!rule!1!handler!in_allow_keepalive = 1
%(vsrv_pre)s!rule!1!handler!in_preserve_host = 1
"""

"""
vserver!120!rule!100!handler!balancer = ip_hash
vserver!120!rule!100!handler!balancer!source!1 = 15
vserver!120!rule!100!handler!in_allow_keepalive = 1
vserver!120!rule!100!handler!in_preserve_host = 1
vserver!120!rule!100!match = default
vserver!120!rule!100!match!final = 1
"""

CONFIG_RULES = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = proxy
%(rule_pre)s!handler!balancer = ip_hash
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
%(rule_pre)s!handler!in_allow_keepalive = 1
%(rule_pre)s!handler!in_preserve_host = 1
"""

def is_coldfusion_host (host):
    try:
        cf_host = urllib.urlopen("http://%s" % host)
    except:
        raise ValueError, _(ERROR_NO_SRC)

    headers = str(cf_host.headers).lower()
    cf_host.close()
    if not "jrun" in headers:
        raise ValueError, _(ERROR_NO_SRC)
    return host

DATA_VALIDATION = [
    ("tmp!wizard_coldfusion!new_host",  (validations.is_new_host, 'cfg')),
    ('tmp!wizard_coldfusion!new_source', validations.is_information_source),
    ('tmp!wizard_coldfusion!new_source', is_coldfusion_host)
]

class Wizard_VServer_ColdFusion (WizardPage):
    ICON = "coldfusion.png"
    DESC = _("New virtual server to proxy a ColdFusion host.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/ColdFusion',
                             id     = "ColdFusion_Page1",
                             title  = _("ColdFusion Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>%s</h2>' % (_("New Virtual Server"))
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_coldfusion!new_host', _(NOTE_VSRV_NAME), value="coldfusion.example.com")
        txt += self.Indent(table)

        txt += '<h2>ColdFusion</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('Host'), 'tmp!wizard_coldfusion!new_source', _(NOTE_SOURCE), value="127.0.0.1:8500")
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Logging"))
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_coldfusion!new_host',   _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_coldfusion!new_source', _(ERROR_EMPTY))

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        new_host   = post.pop('tmp!wizard_coldfusion!new_host')
        new_source = post.pop('tmp!wizard_coldfusion!new_source')

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)
        src_num, src_pre = cfg_source_get_next (self._cfg)

        # Add the new rules
        config = CONFIG_VSRV % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, vsrv_pre)


class Wizard_Rules_ColdFusion (WizardPage):
    ICON = "coldfusion.png"
    DESC = _("New directory to proxy a Coldfusion host.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/ColdFusion'%(pre.split('!')[1]),
                             id     = "Coldfusion_Page1",
                             title  = _("ColdFusion Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>ColdFusion</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'), 'tmp!wizard_coldfusion!new_webdir', _(NOTE_WEB_DIR), value="/coldfusion")
        self.AddPropEntry (table, _('Host'),   'tmp!wizard_coldfusion!new_source', _(NOTE_SOURCE),  value="127.0.0.1:8500")
        txt += self.Indent(table)

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_coldfusion!new_webdir', _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_coldfusion!new_source', _(ERROR_EMPTY))

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        webdir     = post.pop('tmp!wizard_coldfusion!new_webdir')
        new_source = post.pop('tmp!wizard_coldfusion!new_source')

        # Locals
        rule_num, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        src_num,  src_pre  = cfg_source_get_next (self._cfg)

        # Add the new rules
        config = CONFIG_RULES % (locals())
        self._apply_cfg_chunk (config)
