"""
Moinmoin wizard.

Last update:
* Cherokee 0.99.32b
* MoinMoin 1.8.5
"""
import validations
import re

from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

NOTE_MOINMOIN_WIKI = N_("Local path to the MoinMoin Wiki Instance.")
NOTE_NEW_HOST = N_("Name of the new domain that will be created.")
NOTE_WEB_DIR = N_("Web folder under which MoinMoin Wiki will be accessible.")

ERROR_NO_MOINMOIN_WIKI = N_("It does not look like a MoinMoin Wiki Instance.")
ERROR_NO_MOINMOIN = N_("MoinMoin doesn't seem to be correctly installed.")

# TCP port value is automatically asigned to one currently not in use
SOURCE = """
source!%(src_num)d!env_inherited = 1
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = spawn-fcgi -n -a 127.0.0.1 -p %(src_port)d -- %(moinmoin_wiki)s/server/moin.fcg
source!%(src_num)d!nick = MoinMoin %(src_num)d
source!%(src_num)d!type = interpreter
"""

CONFIG_VSRV = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = /dev/null

%(vsrv_pre)s!rule!200!match = directory
%(vsrv_pre)s!rule!200!match!directory = %(moinmoin_static)s
%(vsrv_pre)s!rule!200!match!final = 1
%(vsrv_pre)s!rule!200!document_root = %(moinmoin_wiki)s/htdocs
%(vsrv_pre)s!rule!200!encoder!deflate = 0
%(vsrv_pre)s!rule!200!encoder!gzip = 0
%(vsrv_pre)s!rule!200!handler = file
%(vsrv_pre)s!rule!200!handler!iocache = 1
%(vsrv_pre)s!rule!200!no_log = 0
%(vsrv_pre)s!rule!200!only_secure = 0

%(vsrv_pre)s!rule!100!match = default
%(vsrv_pre)s!rule!100!match!final = 1
%(vsrv_pre)s!rule!100!handler = fcgi
%(vsrv_pre)s!rule!100!handler!balancer = round_robin
%(vsrv_pre)s!rule!100!handler!balancer!source!1 = %(src_num)d
%(vsrv_pre)s!rule!100!handler!change_user = 0
%(vsrv_pre)s!rule!100!handler!check_file = 0
%(vsrv_pre)s!rule!100!handler!error_handler = 1
%(vsrv_pre)s!rule!100!handler!pass_req_headers = 1
%(vsrv_pre)s!rule!100!handler!x_real_ip_access_all = 0
%(vsrv_pre)s!rule!100!handler!x_real_ip_enabled = 0
%(vsrv_pre)s!rule!100!handler!xsendfile = 0
"""

CONFIG_DIR = SOURCE + """
%(rule_pre_2)s!match = directory
%(rule_pre_2)s!document_root = %(moinmoin_wiki)s/htdocs
%(rule_pre_2)s!match!directory = %(web_dir)s%(moinmoin_static)s
%(rule_pre_2)s!match!final = 1
%(rule_pre_2)s!handler = file
%(rule_pre_2)s!handler!iocache = 1

%(rule_pre_1)s!match = directory
%(rule_pre_1)s!match!directory = %(web_dir)s
%(rule_pre_1)s!match!final = 1
%(rule_pre_1)s!handler = fcgi
%(rule_pre_1)s!handler!balancer = round_robin
%(rule_pre_1)s!handler!balancer!source!1 = %(src_num)d
%(rule_pre_1)s!handler!change_user = 0
%(rule_pre_1)s!handler!check_file = 0
%(rule_pre_1)s!handler!error_handler = 1
%(rule_pre_1)s!handler!pass_req_headers = 1
"""

def find_url_prefix_static (path):
    try:
        import MoinMoin.config
        return MoinMoin.config.url_prefix_static
    except ImportError:
        RE_MATCH = """url_prefix_static = '(.*?)'"""
        filename = os.path.join (path, "config/wikiconfig.py")
        regex = re.compile(RE_MATCH, re.DOTALL)
        match = regex.search(open(filename).read())
        if match:
            return match.groups()[0]
        raise ValueError, _(ERROR_NO_MOINMOIN)

def is_moinmoin_wiki (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    manage = os.path.join (path, "config/wikiconfig.py")

    if not os.path.exists (manage):
        raise ValueError, _(ERROR_NO_MOINMOIN_WIKI)
    return path


DATA_VALIDATION = [
    ("tmp!wizard_moinmoin!moinmoin_wiki",   (is_moinmoin_wiki, 'cfg')),
    ("tmp!wizard_moinmoin!new_host",        (validations.is_new_host, 'cfg')),
    ("tmp!wizard_moinmoin!new_webdir",      validations.is_dir_formated)
]

SRC_PATHS = [
    "/usr/share/pyshared/moinmoin",
    "/usr/share/pyshared/MoinMoin",
    "/usr/share/moinmoin",
    "/usr/share/moin",
    "/opt/local/share/moin"
]

class Wizard_VServer_MoinMoin (WizardPage):
    ICON = "moinmoin.png"
    DESC = _("New virtual server based on a Moinmoin Wiki Instance.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/Moinmoin',
                             id     = "Moinmoin_Page1",
                             title  = _("Moinmoin Wizard"),
                             group  = _(WIZARD_GROUP_CMS))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)
        guessed_src = path_find_w_default (SRC_PATHS)

        txt += '<h2>%s</h2>' % (_("New Virtual Server"))
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_moinmoin!new_host', _(NOTE_NEW_HOST), value="moinmoin.example.com")
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Moinmoin Wiki Instance"))
        table = TableProps()
        self.AddPropEntry (table, _('Moinmoin Directory'), 'tmp!wizard_moinmoin!moinmoin_wiki', _(NOTE_MOINMOIN_WIKI), value=guessed_src)
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Logging"))
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        new_host        = post.pop('tmp!wizard_moinmoin!new_host')
        moinmoin_wiki   = post.pop('tmp!wizard_moinmoin!moinmoin_wiki')
        moinmoin_static = find_url_prefix_static (moinmoin_wiki)

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)
        src_num, src_pre = cfg_source_get_next (self._cfg)
        src_port = cfg_source_find_free_port ()

        # Add the new rules
        config = CONFIG_VSRV % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, vsrv_pre)


class Wizard_Rules_MoinMoin (WizardPage):
    ICON = "moinmoin.png"
    DESC = _("Configures a Moinmoin Wiki Instance inside a public web directory.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/Moinmoin'%(pre.split('!')[1]),
                             id     = "Moinmoin_Page1",
                             title  = _("Moinmoin Wizard"),
                             group  = _(WIZARD_GROUP_CMS))

    def show (self):
        return True

    def _render_content (self, url_pre):
        guessed_src = path_find_w_default (SRC_PATHS)

        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'),   'tmp!wizard_moinmoin!web_dir', _(NOTE_WEB_DIR), value="/moinmoin")
        self.AddPropEntry (table, _('Source Directory'),'tmp!wizard_moinmoin!moinmoin_wiki', _(NOTE_MOINMOIN_WIKI), value=guessed_src)

        txt  = '<h1>%s</h1>' % (self.title)
        txt += self.Indent(table)
        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        web_dir         = post.pop('tmp!wizard_moinmoin!web_dir')
        moinmoin_wiki   = post.pop('tmp!wizard_moinmoin!moinmoin_wiki')
        moinmoin_static = find_url_prefix_static (moinmoin_wiki)

        # locals
        rule_n, x = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        if not rule_n:
            return self.report_error (_("Couldn't add a new rule."))

        rule_pre_1   = '%s!rule!%d'%(self._pre, rule_n+1)
        rule_pre_2   = '%s!rule!%d'%(self._pre, rule_n+2)
        src_num, src_pre = cfg_source_get_next (self._cfg)
        src_port = cfg_source_find_free_port ()

        # Add the new rules
        config = CONFIG_DIR % (locals())
        self._apply_cfg_chunk (config)
