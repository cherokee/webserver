"""
Concrete5 wizard. Checked with:
* Cherokee 0.99.28b & Concrete5 3.3.1
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

NOTE_SOURCES  = N_("Path to the directory where the Concrete5 source code is located. (Example: /usr/share/concrete5)")
NOTE_WEB_DIR  = N_("Web directory where you want Concrete5 to be accessible. (Example: /cms)")
NOTE_HOST     = N_("Host name of the virtual host that is about to be created.")
ERROR_NO_SRC  = N_("Does not look like a Concrete5 source directory.")
ERROR_NO_WEB  = N_("A web directory must be provided.")
ERROR_NO_HOST = N_("A host name must be provided.")

CONFIG_DIR = """
%(pre_rule_plus2)s!match = request
%(pre_rule_plus2)s!match!request = ^%(web_dir)s/$
%(pre_rule_plus2)s!handler = redir
%(pre_rule_plus2)s!handler!rewrite!1!show = 0
%(pre_rule_plus2)s!handler!rewrite!1!substring = %(web_dir)s/index.php

%(pre_rule_plus1)s!match = directory
%(pre_rule_plus1)s!match!directory = %(web_dir)s
%(pre_rule_plus1)s!match!final = 0
%(pre_rule_plus1)s!document_root = %(local_src_dir)s

# IMPORTANT: The PHP rule comes here

%(pre_rule_minus1)s!match = and
%(pre_rule_minus1)s!match!left = directory
%(pre_rule_minus1)s!match!left!directory = %(web_dir)s
%(pre_rule_minus1)s!match!right = exists
%(pre_rule_minus1)s!match!right!iocache = 1
%(pre_rule_minus1)s!match!right!match_any = 1
%(pre_rule_minus1)s!handler = file

%(pre_rule_minus2)s!match = directory
%(pre_rule_minus2)s!match!directory = %(web_dir)s
%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!show = 0
%(pre_rule_minus2)s!handler!rewrite!1!regex = /(.*)$
%(pre_rule_minus2)s!handler!rewrite!1!substring = %(web_dir)s/index.php/$1
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!document_root = %(local_src_dir)s
%(pre_vsrv)s!directory_index = index.php,index.html
%(pre_vsrv)s!keepalive = 1

%(pre_rule_plus1)s!handler = file
%(pre_rule_plus1)s!handler!iocache = 1
%(pre_rule_plus1)s!match = exists
%(pre_rule_plus1)s!match!exists = sitemap.xml,robots.txt
%(pre_rule_plus1)s!match!iocache = 0
%(pre_rule_plus1)s!match!match_any = 0
%(pre_rule_plus1)s!match!match_index_files = 1
%(pre_rule_plus1)s!match!match_only_files = 1

# IMPORTANT: The PHP rule comes here

%(pre_rule_minus1)s!handler = file
%(pre_rule_minus1)s!match = directory
%(pre_rule_minus1)s!match!directory = /concrete

%(pre_rule_minus2)s!handler = file
%(pre_rule_minus2)s!match = directory
%(pre_rule_minus2)s!match!directory = /themes

%(pre_rule_minus3)s!handler = file
%(pre_rule_minus3)s!handler!iocache = 1
%(pre_rule_minus3)s!match = directory
%(pre_rule_minus3)s!match!directory = /files

%(pre_rule_minus4)s!match = default
%(pre_rule_minus4)s!handler = redir
%(pre_rule_minus4)s!handler!rewrite!1!show = 0
%(pre_rule_minus4)s!handler!rewrite!1!regex = ^(.*)$
%(pre_rule_minus4)s!handler!rewrite!1!substring = index.php/$1
"""

SRC_PATHS = [
    "/usr/share/concrete5",          # Debian, Fedora
    "/var/www/*/htdocs/concrete5",   # Gentoo
    "/srv/www/htdocs/concrete5",     # SuSE
    "/usr/local/www/data/concrete5*" # BSD
]

def is_concrete5_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    module_inc = os.path.join (path, 'concrete/libraries/loader.php')
    if not os.path.exists (module_inc):
        raise ValueError, _(ERROR_NO_SRC)
    return path

DATA_VALIDATION = [
    ("tmp!wizard_concrete5!sources", (is_concrete5_dir, 'cfg')),
    ("tmp!wizard_concrete5!host",    (validations.is_new_host, 'cfg')),
    ("tmp!wizard_concrete5!web_dir",  validations.is_dir_formated)
]

class Wizard_VServer_Concrete5 (WizardPage):
    ICON = "concrete5.png"
    DESC = _("Configure a new virtual server using Concrete5.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/Concrete5',
                             id     = "Concrete5_Page1",
                             title  = _("Concrete5 Wizard"),
                             group  = _(WIZARD_GROUP_CMS))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt  = '<h1>%s</h1>' % (self.title)
        guessed_src = path_find_w_default (SRC_PATHS)

        txt += '<h2>Concrete5</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'),    'tmp!wizard_concrete5!host',    _(NOTE_HOST),    value="cms.example.com")
        self.AddPropEntry (table, _('Source Directory'), 'tmp!wizard_concrete5!sources', _(NOTE_SOURCES), value=guessed_src)
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Logging"))
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_concrete5!host',    _(ERROR_NO_HOST))
        self.Validate_NotEmpty (post, 'tmp!wizard_concrete5!sources', _(ERROR_NO_SRC))

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        local_src_dir = post.pop('tmp!wizard_concrete5!sources')
        host          = post.pop('tmp!wizard_concrete5!host')
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

        pre_rule_plus1  = "%s!rule!%d" % (pre_vsrv, php_rule + 1)
        pre_rule_minus1 = "%s!rule!%d" % (pre_vsrv, php_rule - 1)
        pre_rule_minus2 = "%s!rule!%d" % (pre_vsrv, php_rule - 2)
        pre_rule_minus3 = "%s!rule!%d" % (pre_vsrv, php_rule - 3)
        pre_rule_minus4 = "%s!rule!%d" % (pre_vsrv, php_rule - 4)

        # Common static
        pre_rule_plus2  = "%s!rule!%d" % (pre_vsrv, php_rule + 2)
        self._common_add_usual_static_files (pre_rule_plus2)

        # Add the new rules
        config = CONFIG_VSERVER % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, pre_vsrv)


class Wizard_Rules_Concrete5 (WizardPage):
    ICON = "concrete5.png"
    DESC = _("Configures Concrete5 inside a public web directory.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/Concrete5'%(pre.split('!')[1]),
                             id     = "Concrete5_Page1",
                             title  = _("Concrete5 Wizard"),
                             group  = _(WIZARD_GROUP_CMS))

    def show (self):
        # Check for PHP
        php_info = wizard_php_get_info (self._cfg, self._pre)
        if not php_info:
            self.no_show = _("PHP support is required.")
            return False
        return True

    def _render_content (self, url_pre):
        guessed_src = path_find_w_default (SRC_PATHS)

        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'),   'tmp!wizard_concrete5!web_dir', _(NOTE_WEB_DIR), value="/cms")
        self.AddPropEntry (table, _('Source Directory'),'tmp!wizard_concrete5!sources', _(NOTE_SOURCES), value=guessed_src)

        txt  = '<h1>%s</h1>' % (self.title)
        txt += self.Indent(table)
        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_concrete5!web_dir', _(ERROR_NO_WEB))
        self.Validate_NotEmpty (post, 'tmp!wizard_concrete5!sources', _(ERROR_NO_SRC))

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        local_src_dir = post.pop('tmp!wizard_concrete5!sources')
        web_dir       = post.pop('tmp!wizard_concrete5!web_dir')

        # Replacement
        php_info = wizard_php_get_info (self._cfg, self._pre)
        php_rule = int (php_info['rule'].split('!')[-1])

        pre_rule_plus2  = "%s!rule!%d" % (self._pre, php_rule + 2)
        pre_rule_plus1  = "%s!rule!%d" % (self._pre, php_rule + 1)
        pre_rule_minus1 = "%s!rule!%d" % (self._pre, php_rule - 1)
        pre_rule_minus2 = "%s!rule!%d" % (self._pre, php_rule - 2)

        # Add the new rules
        config = CONFIG_DIR % (locals())
        self._apply_cfg_chunk (config)

