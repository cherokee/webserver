"""
Trac wizard.

Last update:
* Cherokee 0.99.25
* Trac     0.11.1
"""
import validations

from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

NOTE_TRAC_PROJECT = N_("Local path to the Trac project.")
NOTE_TRAC_DATA = N_("Local path to the Trac installation. (Example: /usr/share/trac)")
NOTE_NEW_HOST = N_("Name of the new domain that will be created.")

ERROR_NO_TRAC_PROJECT = N_("It does not look like a Trac based project directory.")
ERROR_NO_TRAC_DATA = N_("It does not look like a Trac installation.")

# TCP port value is automatically asigned to one currently not in use

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = Trac %(src_num)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = tracd --single-env --daemonize --protocol=scgi --hostname=%(localhost)s --port=%(src_port)s %(trac_project)s
"""

CONFIG_VSRV = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = /dev/null

%(vsrv_pre)s!rule!10!match = directory
%(vsrv_pre)s!rule!10!match!directory = /chrome/common
%(vsrv_pre)s!rule!10!document_root = %(trac_data)s/htdocs
%(vsrv_pre)s!rule!10!handler = file
%(vsrv_pre)s!rule!10!expiration = time
%(vsrv_pre)s!rule!10!expiration!time = 7d

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = scgi
%(vsrv_pre)s!rule!1!handler!change_user = 0
%(vsrv_pre)s!rule!1!handler!check_file = 0
%(vsrv_pre)s!rule!1!handler!error_handler = 0
%(vsrv_pre)s!rule!1!handler!pass_req_headers = 1
%(vsrv_pre)s!rule!1!handler!xsendfile = 0
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
"""

def is_trac_data (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    manage = os.path.join (path, "htdocs")

    if not os.path.exists (manage):
        raise ValueError, _(ERROR_NO_TRAC_DATA)
    return path

def is_trac_project (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    manage = os.path.join (path, "conf/trac.ini")

    if not os.path.exists (manage):
        raise ValueError, _(ERROR_NO_TRAC_PROJECT)
    return path

DATA_VALIDATION = [
    ("tmp!wizard_trac!trac_data",   (is_trac_data, 'cfg')),
    ("tmp!wizard_trac!trac_project",(is_trac_project, 'cfg')),
    ("tmp!wizard_trac!new_host",    (validations.is_new_host, 'cfg')),
    ("tmp!wizard_trac!new_webdir",   validations.is_dir_formated)
]

SRC_PATHS = [
    "/usr/share/pyshared/trac",
    "/usr/share/trac"
]

class Wizard_VServer_Trac (WizardPage):
    ICON = "trac.png"
    DESC = _("New virtual server based on a Trac project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/Trac',
                             id     = "Trac_Page1",
                             title  = _("Trac Wizard"),
                             group  = _(WIZARD_GROUP_CMS))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)
        guessed_src = path_find_w_default (SRC_PATHS)

        txt += '<h2>%s</h2>' % (_("New Virtual Server"))
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_trac!new_host', _(NOTE_NEW_HOST), value="trac.example.com")
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Trac Project"))
        table = TableProps()
        self.AddPropEntry (table, _('Project Directory'), 'tmp!wizard_trac!trac_project', _(NOTE_TRAC_PROJECT), value=os_get_document_root())
        self.AddPropEntry (table, _('Trac Directory'), 'tmp!wizard_trac!trac_data', _(NOTE_TRAC_DATA), value=guessed_src)
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Logging"))
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_trac!trac_data',    _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_trac!trac_project', _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_trac!new_host',     _(ERROR_EMPTY))
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        new_host     = post.pop('tmp!wizard_trac!new_host')
        trac_data    = post.pop('tmp!wizard_trac!trac_data')
        trac_project = post.pop('tmp!wizard_trac!trac_project')

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)
        src_num, src_pre = cfg_source_get_next (self._cfg)
        src_port = cfg_source_find_free_port ()
        localhost = cfg_source_get_localhost_addr()

        # Add the new rules
        config = CONFIG_VSRV % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, vsrv_pre)

