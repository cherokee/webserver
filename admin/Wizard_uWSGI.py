"""
uWSGI wizard.

Last update:
* Cherokee 0.99.36b
* uWSGI Version 0.9.3
"""
import validations
import re

from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

NOTE_UWSGI_CONFIG = N_("Path to the uWSGI configuration file (XML or Python-only). Its mountpoint will be used.")
NOTE_UWSGI_BINARY = N_("Location of the uWSGI binary")

NOTE_NEW_HOST = N_("Name of the new domain that will be created.")
NOTE_NEW_DIR = N_("Public web directory to access the project.")
NOTE_DROOT = N_("Document root for the new virtual server.")

ERROR_NO_UWSGI_CONFIG = N_("It does not look like a uWSGI configuration file.")
ERROR_NO_UWSGI_BINARY = N_("The uWSGI server could not be found.")

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = uWSGI %(src_num)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = %(uwsgi_binary)s -s 127.0.0.1:%(src_port)d -t 10 -M -p 1 -C %(uwsgi_extra)s
"""

SOURCE_XML   = """ -x %s"""
SOURCE_WSGI  = """ -w %s"""
SOURCE_VE    = """ -H %s"""

CONFIG_VSRV = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(document_root)s

%(vsrv_pre)s!rule!2!match = directory
%(vsrv_pre)s!rule!2!match!directory = %(webdir)s
%(vsrv_pre)s!rule!2!encoder!gzip = 1
%(vsrv_pre)s!rule!2!handler = uwsgi
%(vsrv_pre)s!rule!2!handler!error_handler = 1
%(vsrv_pre)s!rule!2!handler!check_file = 0
%(vsrv_pre)s!rule!2!handler!pass_req_headers = 1
%(vsrv_pre)s!rule!2!handler!balancer = round_robin
%(vsrv_pre)s!rule!2!handler!balancer!source!1 = %(src_num)d

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!handler = common
%(vsrv_pre)s!rule!1!handler!iocache = 0
"""

CONFIG_RULES = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = uwsgi
%(rule_pre)s!handler!error_handler = 1
%(rule_pre)s!handler!check_file = 0
%(rule_pre)s!handler!pass_req_headers = 1
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
"""

DEFAULT_BINS   = ['uwsgi','uwsgi26','uwsgi25']

DEFAULT_PATHS  = ['/usr/bin',
                  '/usr/gnu/bin',
                  '/opt/local/bin']

RE_MOUNTPOINT_XML = """<uwsgi>.*?<app mountpoint=["|'](.*?)["|']>.*</uwsgi>"""
RE_MOUNTPOINT_WSGI  = """applications.*=.*{.*'(.*?)':"""

def is_uwsgi_cfg (filename, cfg, nochroot):
    filename = validations.is_local_file_exists (filename, cfg, nochroot)
    mountpoint = find_mountpoint_xml(filename)
    if not mountpoint:
        mountpoint = find_mountpoint_wsgi(filename)
        if not mountpoint:
            raise ValueError, _(ERROR_NO_UWSGI_CONFIG)
    return filename

def find_virtualenv (filename):
    try:
        import virtualenv
    except ImportError:
        return None

    dirname = os.path.dirname(os.path.normpath(filename))
    return os.path.normpath(dirname + '/..')

def find_mountpoint_xml (filename):
    regex = re.compile(RE_MOUNTPOINT_XML, re.DOTALL)
    match = regex.search(open(filename).read())
    if match:
        return match.groups()[0]

def find_mountpoint_wsgi (filename):
    regex = re.compile(RE_MOUNTPOINT_WSGI, re.DOTALL)
    match = regex.search(open(filename).read())
    if match:
        return match.groups()[0]

def find_uwsgi_binary ():
    return path_find_binary (DEFAULT_BINS, extra_dirs = DEFAULT_PATHS)

DATA_VALIDATION = [
    ("tmp!wizard_uwsgi!uwsgi_cfg",     (is_uwsgi_cfg, 'cfg')),
    ("tmp!wizard_uwsgi!new_host",      (validations.is_new_host, 'cfg')),
    ("tmp!wizard_uwsgi!new_webdir",     validations.is_dir_formated),
    ("tmp!wizard_uwsgi!uwsgi_binary",  (validations.is_local_file_exists, 'cfg')),
    ("tmp!wizard_uwsgi!document_root", (validations.is_local_dir_exists, 'cfg'))
]

class Wizard_VServer_uWSGI (WizardPage):
    ICON = "uwsgi.png"
    DESC = _("New virtual server for an uWSGI project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/uWSGI',
                             id     = "uWSGI_Page1",
                             title  = _("uWSGI Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))
        self.uwsgi_binary = find_uwsgi_binary()

    def show (self):
        if not self.uwsgi_binary:
            self.no_show = _(ERROR_NO_UWSGI_BINARY)
            return False
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>%s</h2>' % (_("New Virtual Server"))
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_uwsgi!new_host',      _(NOTE_NEW_HOST), value="www.example.com")
        self.AddPropEntry (table, _('Document Root'), 'tmp!wizard_uwsgi!document_root', _(NOTE_DROOT), value=os_get_document_root())
        txt += self.Indent(table)

        txt += '<h2>uWSGI</h2>'
        table = TableProps()
        if not self.uwsgi_binary:
            self.AddPropEntry (table, _('uWSGI binary'), 'tmp!wizard_uwsgi!uwsgi_binary', _(NOTE_UWSGI_BINARY))
        self.AddPropEntry (table, _('Configuration File'), 'tmp!wizard_uwsgi!uwsgi_cfg', _(NOTE_UWSGI_CONFIG))
        txt += self.Indent(table)

        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_uwsgi!document_root', _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_uwsgi!new_host',      _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_uwsgi!uwsgi_cfg',     _(ERROR_EMPTY))
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        new_host      = post.pop('tmp!wizard_uwsgi!new_host')
        document_root = post.pop('tmp!wizard_uwsgi!document_root')
        uwsgi_cfg     = post.pop('tmp!wizard_uwsgi!uwsgi_cfg')
        webdir        = find_mountpoint_xml(uwsgi_cfg)

        # Choose between XML config+launcher, or Python only config
        if webdir:
            uwsgi_extra = SOURCE_XML % uwsgi_cfg
        else:
            webdir = find_mountpoint_wsgi(uwsgi_cfg)
            uwsgi_extra = SOURCE_WSGI % uwsgi_cfg

        # Virtualenv support
        uwsgi_virtualenv = find_virtualenv(uwsgi_cfg)
        if uwsgi_virtualenv:
            uwsgi_extra += SOURCE_VE % uwsgi_virtualenv

        uwsgi_binary  = self.uwsgi_binary
        if not uwsgi_binary:
            uwsgi_binary = post.pop('tmp!wizard_uwsgi!uwsgi_binary')

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


class Wizard_Rules_uWSGI (WizardPage):
    ICON = "uwsgi.png"
    DESC = _("New directory for an uWSGI project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/uWSGI'%(pre.split('!')[1]),
                             id     = "uWSGI_Page1",
                             title  = _("uWSGI Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))
        self.uwsgi_binary = find_uwsgi_binary()

    def show (self):
        if not self.uwsgi_binary:
            self.no_show = _(ERROR_NO_UWSGI_BINARY)
            return False
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>uWSGI</h2>'
        table = TableProps()
        if not self.uwsgi_binary:
            self.AddPropEntry (table, _('uWSGI binary'), 'tmp!wizard_uwsgi!uwsgi_binary', _(NOTE_UWSGI_BINARY))
        self.AddPropEntry (table, _('Configuration File'), 'tmp!wizard_uwsgi!uwsgi_cfg', _(NOTE_UWSGI_CONFIG))
        txt += self.Indent(table)

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_uwsgi!uwsgi_cfg',     _(ERROR_EMPTY))
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        uwsgi_cfg     = post.pop('tmp!wizard_uwsgi!uwsgi_cfg')
        webdir        = find_mountpoint_xml(uwsgi_cfg)

        # Choose between XML config+launcher, or Python only config
        if webdir:
            uwsgi_extra = SOURCE_XML % uwsgi_cfg
        else:
            webdir = find_mountpoint_wsgi(uwsgi_cfg)
            uwsgi_extra = SOURCE_WSGI % uwsgi_cfg

        # Virtualenv support
        uwsgi_virtualenv = find_virtualenv(uwsgi_cfg)
        if uwsgi_virtualenv:
            uwsgi_extra += SOURCE_VE % uwsgi_virtualenv

        uwsgi_binary  = self.uwsgi_binary
        if not uwsgi_binary:
            uwsgi_binary = post.pop('tmp!wizard_uwsgi!uwsgi_binary')

        # Locals
        rule_num, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        src_num,  src_pre  = cfg_source_get_next (self._cfg)
        src_port = cfg_source_find_free_port ()

        # Add the new rules
        config = CONFIG_RULES % (locals())
        self._apply_cfg_chunk (config)
