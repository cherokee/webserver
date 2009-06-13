import validations

from config import *
from util import *
from Page import *
from Wizard import *

ROR_CHILD_PROCS = 3
DEFAULT_BINS    = ['spawn-fcgi']

NOTE_ROR_DIR   = _("Local path to the Ruby on Rails based project.")
NOTE_NEW_HOST  = _("Name of the new domain that will be created.")
NOTE_NEW_DIR   = _("Directory of the web directory where the Ruby on Rails project will live in.")

ERROR_NO_ROR   = _("It does not look like a Ruby on Rails based project directory.")
ERROR_NO_DROOT = _("The document root directory does not exist.")

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = RoR %(new_host)s, instance %(src_instance)d
source!%(src_num)d!host = /tmp/cherokee-ror-%(src_num)d.sock
source!%(src_num)d!interpreter = spawn-fcgi -n -f %(ror_dir)s/public/dispatch.fcgi -s /tmp/cherokee-ror-%(src_num)d.sock -P /tmp/cherokee-ror-%(src_num)d.sock
"""

CONFIG_VSRV = """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(ror_dir)s/public
%(vsrv_pre)s!directory_index = index.html

%(vsrv_pre)s!rule!10!match = exists
%(vsrv_pre)s!rule!10!match!match_any = 1
%(vsrv_pre)s!rule!10!match!match_only_files = 1
%(vsrv_pre)s!rule!10!match!match_index_files = 1
%(vsrv_pre)s!rule!10!handler = common
%(vsrv_pre)s!rule!10!expiration = time
%(vsrv_pre)s!rule!10!expiration!time = 7d

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = fcgi
%(vsrv_pre)s!rule!1!handler!error_handler = 1
%(vsrv_pre)s!rule!1!handler!check_file = 0
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
"""

CONFIG_VSRV_CHILD = """
%(vsrv_pre)s!rule!1!handler!balancer!source!%(src_instance)d = %(src_num)d
"""

CONFIG_RULES = """
%(rule_pre_plus2)s!match = directory
%(rule_pre_plus2)s!match!directory = %(webdir)s
%(rule_pre_plus2)s!document_root = %(ror_dir)s/public
%(rule_pre_plus2)s!final = 0

%(rule_pre_plus1)s!match = and
%(rule_pre_plus1)s!match!left = directory
%(rule_pre_plus1)s!match!left!directory = %(webdir)s
%(rule_pre_plus1)s!match!right = exists
%(rule_pre_plus1)s!match!right!any_file = 1
%(rule_pre_plus1)s!match!right!match_only_files = 0
%(rule_pre_plus1)s!handler = common
%(rule_pre_plus1)s!expiration = time
%(rule_pre_plus1)s!expiration!time = 7d

%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = fcgi
%(rule_pre)s!handler!error_handler = 1
%(rule_pre)s!handler!check_file = 0
%(rule_pre)s!handler!balancer = round_robin
"""

CONFIG_RULES_CHILD = """
%(rule_pre)s!handler!balancer!source!%(src_instance)d = %(src_num)d
"""

def is_ror_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    manage = os.path.join (path, "script/process/spawner")
    if not os.path.exists (manage):
        raise ValueError, _("Directory doesn't look like a Ruby on Rails based project.")
    return path

DATA_VALIDATION = [
    ("tmp!wizard_ror!ror_dir",  (is_ror_dir, 'cfg')),
    ("tmp!wizard_ror!new_host", (validations.is_new_host, 'cfg'))
]

class Wizard_VServer_RoR (WizardPage):
    ICON = "ror.png"
    DESC = "New virtual server based on a Ruby on Rails project."

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/RoR',
                             id     = "RoR_Page1",
                             title  = _("Ruby on Rails Wizard"),
                             group  = WIZARD_GROUP_PLATFORM)

    def show (self):
        spawn_fcgi = path_find_binary (DEFAULT_BINS)
        if not spawn_fcgi:
            self.no_show = "Could not find the spawn-fcgi binary"
            return False
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>New Virtual Server</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_ror!new_host',      NOTE_NEW_HOST, value="www.example.com")
        txt += self.Indent(table)

        txt += '<h2>Ruby on Rails Project</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('Project Directory'), 'tmp!wizard_ror!ror_dir', NOTE_ROR_DIR)
        txt += self.Indent(table)

        txt += '<h2>Logging</h2>'
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Validation
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        # Incoming info
        ror_dir  = post.pop('tmp!wizard_ror!ror_dir')
        new_host = post.pop('tmp!wizard_ror!new_host')

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)
        src_num, src_pre = cfg_source_get_next (self._cfg)

        # Usual Static files
        self._common_add_usual_static_files ("%s!rule!500" % (vsrv_pre))

        # Add the new main rules
        config = CONFIG_VSRV % (locals())

        # Add the Information Sources
        for i in range(ROR_CHILD_PROCS):
            src_instance = i + 1
            config += SOURCE % (locals())
            config += CONFIG_VSRV_CHILD % (locals())
            src_num += 1

        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, vsrv_pre)


class Wizard_Rules_RoR (WizardPage):
    ICON = "ror.png"
    DESC = "New directory based on a Ruby of Rails project."

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre, 
                             submit = '/vserver/%s/wizard/RoR'%(pre.split('!')[1]),
                             id     = "RoR_Page1",
                             title  = _("Ruby on Rails Wizard"),
                             group  = WIZARD_GROUP_PLATFORM)

    def show (self):
        spawn_fcgi = path_find_binary (DEFAULT_BINS)
        if not spawn_fcgi:
            self.no_show = "Could not find the spawn-fcgi binary"
            return False
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>Web Directory</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'), 'tmp!wizard_ror!new_webdir', NOTE_NEW_DIR, value="/project")
        txt += self.Indent(table)

        txt += '<h2>Ruby on Rails Project</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('Project Directory'), 'tmp!wizard_ror!ror_dir', NOTE_ROR_DIR)
        txt += self.Indent(table)

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Validation
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        # Incoming info
        ror_dir = post.pop('tmp!wizard_ror!ror_dir')
        webdir  = post.pop('tmp!wizard_ror!new_webdir')

        # Locals
        rule_num, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        src_num,  src_pre  = cfg_source_get_next (self._cfg)
        new_host           = self._cfg.get_val ("%s!nick"%(self._pre))
        rule_pre_plus2     = "%s!rule!%d" % (self._pre, rule_num + 2)
        rule_pre_plus1     = "%s!rule!%d" % (self._pre, rule_num + 1)

        # Add the new rules
        config = CONFIG_RULES % (locals())

        # Add the Information Sources
        for i in range(ROR_CHILD_PROCS):
            src_instance = i + 1
            config += SOURCE % (locals())
            config += CONFIG_RULES_CHILD % (locals())
            src_num += 1

        self._apply_cfg_chunk (config)

