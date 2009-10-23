"""
Zend wizard. Checked with:
* Cherokee 0.99.25
* Zend     1.9.3
"""
import validations
from config import *
from util import *
from Page import *
from Wizard import *
from Wizard_PHP import wizard_php_get_info
from Wizard_PHP import wizard_php_get_source_info

# For gettext
N_ = lambda x: x

NOTE_SOURCES  = N_("Path to the directory where the Zend source code is located. (Example: /usr/share/zend)")
NOTE_WEB_DIR  = N_("Web directory where you want Zend to be accessible. (Example: /blog)")
NOTE_HOST     = N_("Host name of the virtual host that is about to be created.")

CONFIG_DIR = """
# PHP rule

%(pre_rule_minus1)s!document_root = %(local_src_dir)s
%(pre_rule_minus1)s!match = directory
%(pre_rule_minus1)s!match!directory = %(web_dir)s
%(pre_rule_minus1)s!match!final = 0

%(pre_rule_minus2)s!match = and
%(pre_rule_minus2)s!match!final = 1
%(pre_rule_minus2)s!match!left = directory
%(pre_rule_minus2)s!match!left!directory = %(web_dir)s
%(pre_rule_minus2)s!match!right = exists
%(pre_rule_minus2)s!match!right!iocache = 1
%(pre_rule_minus2)s!match!right!match_any = 1
%(pre_rule_minus2)s!handler = file
%(pre_rule_minus2)s!handler!iocache = 1

%(pre_rule_minus3)s!match = request
%(pre_rule_minus3)s!match!request = %(web_dir)s/.+
%(pre_rule_minus3)s!handler = redir
%(pre_rule_minus3)s!handler!rewrite!1!show = 0
%(pre_rule_minus3)s!handler!rewrite!1!substring = %(web_dir)s/index.php
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!document_root = %(local_src_dir)s
%(pre_vsrv)s!directory_index = index.php,index.html

# PHP rule

%(pre_rule_minus1)s!handler = common
%(pre_rule_minus1)s!match = fullpath
%(pre_rule_minus1)s!match!fullpath!1 = /

%(pre_rule_minus2)s!handler = common
%(pre_rule_minus2)s!match = exists
%(pre_rule_minus2)s!match!match_any = 1

%(pre_rule_minus3)s!handler = redir
%(pre_rule_minus3)s!handler!rewrite!1!regex = ^.*$
%(pre_rule_minus3)s!handler!rewrite!1!show = 0
%(pre_rule_minus3)s!handler!rewrite!1!substring = /index.php
%(pre_rule_minus3)s!match = default
%(pre_rule_minus3)s!match!final = 1
"""


DATA_VALIDATION = [
    ("tmp!wizard_zend!sources", (validations.is_local_dir_exists, 'cfg')),
    ("tmp!wizard_zend!host",    (validations.is_new_host, 'cfg')),
    ("tmp!wizard_zend!web_dir",  validations.is_dir_formated)
]

class Wizard_VServer_Zend (WizardPage):
    ICON = "zend.png"
    DESC = _("Configures Zend in a new virtual server.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/Zend',
                             id     = "Zend_Page1",
                             title  = _("Zend Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt  = '<h1>%s</h1>' % (self.title)

        txt += '<h2>Zend</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'),    'tmp!wizard_zend!host', _(NOTE_HOST), value="zend.example.com")
        self.AddPropEntry (table, _('Source Directory'), 'tmp!wizard_zend!sources', _(NOTE_SOURCES), value="/var/www")
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Logging"))
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
        local_src_dir = post.pop('tmp!wizard_zend!sources')
        host          = post.pop('tmp!wizard_zend!host')
        pre_vsrv      = cfg_vsrv_get_next (self._cfg)

        # Add PHP Rule
        from Wizard_PHP import Wizard_Rules_PHP
        php_wizard = Wizard_Rules_PHP (self._cfg, pre_vsrv)
        php_wizard.show()
        php_wizard.run (pre_vsrv, None)

        # Replacement
        php_info = wizard_php_get_info (self._cfg, pre_vsrv)
        if not php_info:
            return self.report_error (_("Couldn't find a suitable PHP interpreter."))
        php_rule = int (php_info['rule'].split('!')[-1])

        pre_rule_minus1 = "%s!rule!%d" % (pre_vsrv, php_rule - 1)
        pre_rule_minus2 = "%s!rule!%d" % (pre_vsrv, php_rule - 2)
        pre_rule_minus3 = "%s!rule!%d" % (pre_vsrv, php_rule - 3)

        # Add the new rules
        config = CONFIG_VSERVER % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, pre_vsrv)


class Wizard_Rules_Zend (WizardPage):
    ICON = "zend.png"
    DESC = _("Configures Zend inside a public web directory.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/Zend'%(pre.split('!')[1]),
                             id     = "Zend_Page1",
                             title  = _("Zend Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def show (self):
        # Check for PHP
        php_info = wizard_php_get_info (self._cfg, self._pre)
        if not php_info:
            self.no_show = _("PHP support is required.")
            return False
        return True

    def _render_content (self, url_pre):
        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'),   'tmp!wizard_zend!web_dir', _(NOTE_WEB_DIR), value="/zend")
        self.AddPropEntry (table, _('Source Directory'),'tmp!wizard_zend!sources', _(NOTE_SOURCES), value="/var/www")

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
        local_src_dir = post.pop('tmp!wizard_zend!sources')
        web_dir       = post.pop('tmp!wizard_zend!web_dir')

        # Replacement
        php_info = wizard_php_get_info (self._cfg, self._pre)
        php_rule = int (php_info['rule'].split('!')[-1])

        pre_rule_minus1 = "%s!rule!%d" % (self._pre, php_rule - 1)
        pre_rule_minus2 = "%s!rule!%d" % (self._pre, php_rule - 2)
        pre_rule_minus3 = "%s!rule!%d" % (self._pre, php_rule - 3)

        # Add the new rules
        config = CONFIG_DIR % (locals())
        self._apply_cfg_chunk (config)
