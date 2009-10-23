import validations

from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

NOTE_DJANGO_DIR = N_("Local path to the Django based project.")
NOTE_NEW_HOST   = N_("Name of the new domain that will be created.")

ERROR_NO_DJANGO = N_("It does not look like a Django based project directory.")
ERROR_NO_DROOT  = N_("The document root directory does not exist.")

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = Django %(src_num)d
source!%(src_num)d!host = /tmp/cherokee-source%(src_num)d.sock
source!%(src_num)d!interpreter = python %(django_dir)s/manage.py runfcgi protocol=scgi socket=/tmp/cherokee-source%(src_num)d.sock 
"""

CONFIG_VSRV = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(document_root)s

%(vsrv_pre)s!rule!10!match = directory
%(vsrv_pre)s!rule!10!match!directory = /media
%(vsrv_pre)s!rule!10!handler = file
%(vsrv_pre)s!rule!10!expiration = time
%(vsrv_pre)s!rule!10!expiration!time = 7d

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = scgi
%(vsrv_pre)s!rule!1!handler!error_handler = 1
%(vsrv_pre)s!rule!1!handler!check_file = 0
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
"""

CONFIG_RULES = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = scgi
%(rule_pre)s!handler!error_handler = 1
%(rule_pre)s!handler!check_file = 0
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
"""

def is_django_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    manage = os.path.join (path, "manage.py")
    if not os.path.exists (manage):
        raise ValueError, _("Directory doesn't look like a Django based project.")
    return path

DATA_VALIDATION = [
    ("tmp!wizard_django!django_dir",    (is_django_dir, 'cfg')),
    ("tmp!wizard_django!new_host",      (validations.is_new_host, 'cfg')),
    ("tmp!wizard_django!new_webdir",     validations.is_dir_formated),
    ("tmp!wizard_django!document_root", (validations.is_local_dir_exists, 'cfg'))
]

class Wizard_VServer_Django (WizardPage):
    ICON = "django.png"
    DESC = _("New virtual server based on a Django project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/Django',
                             id     = "Django_Page1",
                             title  = _("Django Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>%s</h2>' % (_("New Virtual Server"))
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_django!new_host',      _(NOTE_NEW_HOST),   value="www.example.com")
        self.AddPropEntry (table, _('Document Root'), 'tmp!wizard_django!document_root', _(NOTE_DJANGO_DIR), value=os_get_document_root())
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Django Project"))
        table = TableProps()
        self.AddPropEntry (table, _('Project Directory'), 'tmp!wizard_django!django_dir', _(NOTE_DJANGO_DIR))
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
        new_host      = post.pop('tmp!wizard_django!new_host')
        document_root = post.pop('tmp!wizard_django!document_root')
        django_dir    = post.pop('tmp!wizard_django!django_dir')

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)
        src_num, src_pre = cfg_source_get_next (self._cfg)

        # Usual Static files
        self._common_add_usual_static_files ("%s!rule!500" % (vsrv_pre))

        # Add the new rules
        config = CONFIG_VSRV % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, vsrv_pre)


class Wizard_Rules_Django (WizardPage):
    ICON = "django.png"
    DESC = _("New directory based on a Django project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/Django'%(pre.split('!')[1]),
                             id     = "Django_Page1",
                             title  = _("Django Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>%s</h2>' % (_("Web Directory"))
        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'), 'tmp!wizard_django!new_webdir', _(NOTE_NEW_HOST), value="/project")
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Django Project"))
        table = TableProps()
        self.AddPropEntry (table, _('Project Directory'), 'tmp!wizard_django!django_dir', _(NOTE_DJANGO_DIR))
        txt += self.Indent(table)

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
        django_dir = post.pop('tmp!wizard_django!django_dir')
        webdir     = post.pop('tmp!wizard_django!new_webdir')

        # Locals
        rule_num, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        src_num,  src_pre  = cfg_source_get_next (self._cfg)

        # Add the new rules
        config = CONFIG_RULES % (locals())
        self._apply_cfg_chunk (config)

