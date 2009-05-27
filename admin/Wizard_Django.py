from config import *
from util import *
from Page import *
from Wizard import *

NOTE_DJANGO_DIR = "Local path to the Django based project."
NOTE_NEW_HOST   = "Name of the new domain that will be created."

CONFIG_VSRV = """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(document_root)s

source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = Django %(src_num)d
source!%(src_num)d!host = /tmp/cherokee-source%(src_num)d.sock
source!%(src_num)d!interpreter = python %(django_dir)s/manage.py runfcgi protocol=scgi socket=/tmp/cherokee-source%(src_num)d.sock 

%(vsrv_pre)s!rule!10!match = directory
%(vsrv_pre)s!rule!10!match!directory = /media
%(vsrv_pre)s!rule!10!handler = file

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = scgi
%(vsrv_pre)s!rule!1!handler!error_handler = 1
%(vsrv_pre)s!rule!1!handler!check_file = 0
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
"""

class Wizard_VServer_Django (WizardPage):
    ICON = "django.png"
    DESC = "New virtual server based on a Django project."

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre, 
                             id    = "Django_Page1",
                             title = _("Django Wizard"))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>New Virtual Server</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_django!new_host',      NOTE_NEW_HOST, value="www.example.com")
        self.AddPropEntry (table, _('Document Root'), 'tmp!wizard_django!document_root', NOTE_NEW_HOST, value=os_get_document_root())
        txt += self.Indent(table)

        txt += '<h2>Django Project</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('Project Directory'), 'tmp!wizard_django!django_dir', NOTE_DJANGO_DIR)
        txt += self.Indent(table)

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Incoming info
        django_dir    = post.pop('tmp!wizard_django!django_dir')
        new_host      = post.pop('tmp!wizard_django!new_host')
        document_root = post.pop('tmp!wizard_django!document_root')

        if not new_host or not django_dir or not document_root:
            return 

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)
        src_num, src_pre  = cfg_source_get_next (self._cfg)

        # Add the new rules
        config = CONFIG_VSRV % (locals())
        self._apply_cfg_chunk (config)

