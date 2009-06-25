import validations

from config import *
from util import *
from Page import *
from Wizard import *

NOTE_MAILMAN_CGI_DIR  = _("Local path to the Mailman CGI directory.")
NOTE_MAILMAN_DATA_DIR = _("Local path to the Mailman data directory.")
NOTE_MAILMAN_ARCH_DIR = _("Local path to the Mailman mail archive directory.")
NOTE_NEW_HOST         = _("Name of the new domain that will be created.")

CONFIG_VSRV = """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = /dev/null

%(vsrv_pre)s!rule!400!match = directory
%(vsrv_pre)s!rule!400!match!directory = /pipermail
%(vsrv_pre)s!rule!400!match!final = 1
%(vsrv_pre)s!rule!400!document_root = %(mailman_arch_dir)s/archives/public
%(vsrv_pre)s!rule!400!encoder!gzip = 1
%(vsrv_pre)s!rule!400!handler = common
%(vsrv_pre)s!rule!400!handler!allow_dirlist = 1

%(vsrv_pre)s!rule!300!match = directory
%(vsrv_pre)s!rule!300!match!directory = /icons
%(vsrv_pre)s!rule!300!match!final = 1
%(vsrv_pre)s!rule!300!document_root = %(mailman_data_dir)s/icons
%(vsrv_pre)s!rule!300!handler = file
%(vsrv_pre)s!rule!300!handler!iocache = 1

%(vsrv_pre)s!rule!200!match = fullpath
%(vsrv_pre)s!rule!200!match!final = 1
%(vsrv_pre)s!rule!200!match!fullpath!1 = /
%(vsrv_pre)s!rule!200!encoder!gzip = 1
%(vsrv_pre)s!rule!200!handler = redir
%(vsrv_pre)s!rule!200!handler!rewrite!1!show = 1
%(vsrv_pre)s!rule!200!handler!rewrite!1!substring = /listinfo

%(vsrv_pre)s!rule!100!match = default
%(vsrv_pre)s!rule!100!match!final = 1
%(vsrv_pre)s!rule!100!document_root = %(mailman_cgi_dir)s
%(vsrv_pre)s!rule!100!encoder!gzip = 1
%(vsrv_pre)s!rule!100!handler = cgi
%(vsrv_pre)s!rule!100!handler!change_user = 1
%(vsrv_pre)s!rule!100!handler!check_file = 1
%(vsrv_pre)s!rule!100!handler!error_handler = 1
%(vsrv_pre)s!rule!100!handler!pass_req_headers = 1
%(vsrv_pre)s!rule!100!handler!xsendfile = 0
"""

SRC_PATHS_CGI = [
    "/usr/local/mailman/cgi-bin",
    "/usr/lib/cgi-bin/mailman",
    "/opt/mailman*/cgi-bin"
]

SRC_PATHS_DATA = [
    "/usr/local/mailman",
    "/usr/share/mailman",
    "/opt/mailman*"
]

SRC_PATHS_ARCH = [
    "/usr/local/mailman",
    "/var/share/mailman",
    "/opt/mailman*"
]

def is_mailman_data_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    file = os.path.join (path, "bin/newlist")
    if not os.path.exists (file):
        raise ValueError, _("It doesn't look like a Mailman data directory.")
    return path
    
def is_mailman_cgi_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    file = os.path.join (path, "listinfo")
    if not os.path.exists (file):
        raise ValueError, _("It doesn't look like a Mailman CGI directory.")
    return path

def is_mailman_arch_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    file = os.path.join (path, "archives/public")
    if not os.path.exists (file):
        raise ValueError, _("It doesn't look like a Mailman archive directory.")
    return path


DATA_VALIDATION = [
    ("tmp!wizard_mailman!new_host",         (validations.is_new_host, 'cfg')),
    ("tmp!wizard_mailman!mailman_data_dir", (is_mailman_data_dir, 'cfg')),
    ("tmp!wizard_mailman!mailman_cgi_dir",  (is_mailman_cgi_dir,  'cfg')),
    ("tmp!wizard_mailman!mailman_arch_dir", (is_mailman_arch_dir, 'cfg')),
]


class Wizard_VServer_Mailman (WizardPage):
    ICON = "mailman.png"
    DESC = "New virtual server based on a Mailman mailing list manager."

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/mailman',
                             id     = "mailman_Page1",
                             title  = _("Mailman Wizard"),
                             group  = WIZARD_GROUP_MISC)

    def show (self):
        return True

    def _render_content (self, url_pre):
        guessed_cgi  = path_find_w_default (SRC_PATHS_CGI)
        guessed_data = path_find_w_default (SRC_PATHS_DATA)
        guessed_arch = path_find_w_default (SRC_PATHS_ARCH)

        txt = '<h1>%s</h1>' % (self.title)
        txt += '<h2>New Virtual Server</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_mailman!new_host', NOTE_NEW_HOST, value="www.example.com")
        txt += self.Indent(table)

        txt += '<h2>Mailman</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('Mailman CGI directory'),  'tmp!wizard_mailman!mailman_cgi_dir',  NOTE_MAILMAN_CGI_DIR,  value=guessed_cgi)
        self.AddPropEntry (table, _('Mailman Data directory'), 'tmp!wizard_mailman!mailman_data_dir', NOTE_MAILMAN_DATA_DIR, value=guessed_data)
        self.AddPropEntry (table, _('Mail Archive directory'), 'tmp!wizard_mailman!mailman_arch_dir', NOTE_MAILMAN_ARCH_DIR, value=guessed_arch)
        txt += self.Indent(table)

        txt += '<h2>Logging</h2>'
        txt += self._common_add_logging()

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
        mailman_cgi_dir  = post.pop('tmp!wizard_mailman!mailman_cgi_dir')
        mailman_data_dir = post.pop('tmp!wizard_mailman!mailman_data_dir')
        mailman_arch_dir = post.pop('tmp!wizard_mailman!mailman_arch_dir')
        new_host         = post.pop('tmp!wizard_mailman!new_host')

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)

        # Add the new main rules
        config = CONFIG_VSRV % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, vsrv_pre)
