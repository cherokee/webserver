import validations

from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

NOTE_MONO_DIR = N_("Local path to the Mono based project.")
NOTE_NEW_HOST   = N_("Name of the new domain that will be created.")
NOTE_WEB_DIR   = N_("Web directory under which the Mono application will be published.")
NOTE_DROOT = N_("Document root (for non Mono related files).")

ERROR_NO_MONO = N_("Directory doesn't look like a Mono based project.")

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = Mono %(src_num)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = %(mono_bin)s --socket=tcp --address=127.0.0.1 --port=%(src_port)d --applications=%(webdir)s:%(mono_dir)s
"""

CONFIG_VSRV = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(document_root)s
%(vsrv_pre)s!directory_index = index.aspx,default.aspx

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!match!final = 1
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = fcgi
%(vsrv_pre)s!rule!1!handler!check_file = 1
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
"""

CONFIG_RULES = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = fcgi
%(rule_pre)s!handler!check_file = 1
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
"""

DEFAULT_BINS  = ['fastcgi-mono-server2','fastcgi-mono-server']

DEFAULT_PATHS = ['/usr/bin',
                 '/usr/sfw/bin',
                 '/usr/gnu/bin',
                 '/opt/local/bin']

def is_mono_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    index = os.path.join (path, "index.aspx")
    default = os.path.join (path, "default.aspx")
    if not os.path.exists (index) and not os.path.exists (default):
        raise ValueError, _(ERROR_NO_MONO)
    return path

DATA_VALIDATION = [
    ("tmp!wizard_mono!mono_dir",      (is_mono_dir, 'cfg')),
    ("tmp!wizard_mono!new_host",      (validations.is_new_host, 'cfg')),
    ("tmp!wizard_mono!new_webdir",     validations.is_dir_formated),
    ("tmp!wizard_mono!document_root", (validations.is_local_dir_exists, 'cfg'))
]

class CommonMethods:
    def show (self):
        self._mono_path = path_find_binary (DEFAULT_BINS,
                                      extra_dirs  = DEFAULT_PATHS )

        if not self._mono_path:
            self.no_show = _("Could not find the fastcgi Mono server binary")
            return False
        return True

class Wizard_VServer_Mono (CommonMethods, WizardPage):
    ICON = "mono.png"
    DESC = _("New virtual server based on a .NET/Mono project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/Mono',
                             id     = "Mono_Page1",
                             title  = _(".NET/Mono Wizard"),
                             group  = _(WIZARD_GROUP_LANGS))

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>%s</h2>' % (_("New Virtual Server"))
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_mono!new_host',      _(NOTE_NEW_HOST),   value="www.example.com")
        self.AddPropEntry (table, _('Document Root'), 'tmp!wizard_mono!document_root', _(NOTE_DROOT), value=os_get_document_root())
        txt += self.Indent(table)

        title = _('Mono Project')
        txt += '<h2>%s</h2>' % title
        table = TableProps()
        self.AddPropEntry (table, _('Project Directory'), 'tmp!wizard_mono!mono_dir', _(NOTE_MONO_DIR))
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Logging"))
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_mono!new_host',      _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_mono!document_root', _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_mono!mono_dir',      _(ERROR_EMPTY))

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        new_host      = post.pop('tmp!wizard_mono!new_host')
        document_root = post.pop('tmp!wizard_mono!document_root')
        mono_dir      = post.pop('tmp!wizard_mono!mono_dir')
        webdir        = '/'
        mono_bin      = self._mono_path

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)
        src_num, src_pre = cfg_source_get_next (self._cfg)
        src_port = cfg_source_find_free_port ()

        # Usual Static files
        self._common_add_usual_static_files ("%s!rule!500" % (vsrv_pre))

        # Add the new rules
        config = CONFIG_VSRV % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, vsrv_pre)


class Wizard_Rules_Mono (CommonMethods, WizardPage):
    ICON = "mono.png"
    DESC = _("New directory based on a .NET/Mono project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/Mono'%(pre.split('!')[1]),
                             id     = "Mono_Page1",
                             title  = _(".NET/Mono Wizard"),
                             group  = _(WIZARD_GROUP_LANGS))

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        title = _('Web Directory')
        txt += '<h2>%s</h2>' % title
        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'), 'tmp!wizard_mono!new_webdir', _(NOTE_WEB_DIR), value="/project")
        txt += self.Indent(table)

        title = _('Mono Project')
        txt += '<h2>%s</h2>' % title
        table = TableProps()
        self.AddPropEntry (table, _('Project Directory'), 'tmp!wizard_mono!mono_dir', _(NOTE_MONO_DIR))
        txt += self.Indent(table)

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_mono!new_webdir',  _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_mono!mono_dir',    _(ERROR_EMPTY))

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        mono_dir = post.pop('tmp!wizard_mono!mono_dir')
        webdir   = post.pop('tmp!wizard_mono!new_webdir')
        mono_bin = self._mono_path

        # Locals
        rule_num, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        src_num,  src_pre  = cfg_source_get_next (self._cfg)
        src_port = cfg_source_find_free_port ()

        # Add the new rules
        config = CONFIG_RULES % (locals())
        self._apply_cfg_chunk (config)

