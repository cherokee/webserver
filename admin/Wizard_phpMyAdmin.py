"""
phpMyAdmin wizard. Checked with:
* Cherokee   0.99.25
* phpMyAdmin 3.1.2
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

NOTE_SOURCES  = N_("Path to the directory where the phpMyAdmin source code is located. (Example: /usr/share/phpmyadmin)")
NOTE_WEB_DIR  = N_("Web directory where you want phpMyAdmin to be accessible. (Example: /phpmyadmin)")
ERROR_NO_SRC  = N_("Does not look like a phpMyAdmin source directory.")
ERROR_NO_WEB  = N_("A web directory must be provided.")

CONFIG_DIR = """
%(pre_rule_plus2)s!handler = custom_error
%(pre_rule_plus2)s!handler!error = 403
%(pre_rule_plus2)s!match = or
%(pre_rule_plus2)s!match!left = directory
%(pre_rule_plus2)s!match!left!directory = %(web_dir)s/libraries
%(pre_rule_plus2)s!match!right = directory
%(pre_rule_plus2)s!match!right!directory = %(web_dir)s/setup/lib

%(pre_rule_plus1)s!match = directory
%(pre_rule_plus1)s!match!directory = %(web_dir)s
%(pre_rule_plus1)s!match!final = 0
%(pre_rule_plus1)s!document_root = %(local_src_dir)s

# IMPORTANT: The PHP rule comes here
"""

SRC_PATHS = [
    "/usr/share/phpmyadmin",          # Debian, Fedora
    "/opt/local/www/phpmyadmin"       # MacPorts
]

def is_phpmyadmin_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    module_inc = os.path.join (path, 'libraries/common.inc.php')
    if not os.path.exists (module_inc):
        raise ValueError, ERROR_NO_SRC
    return path

DATA_VALIDATION = [
    ("tmp!wizard_phpmyadmin!sources", (is_phpmyadmin_dir, 'cfg')),
    ("tmp!wizard_phpmyadmin!web_dir",  validations.is_dir_formated)
]

class Wizard_Rules_phpMyAdmin (WizardPage):
    ICON = "phpmyadmin.png"
    DESC = _("Configures phpMyAdmin inside a public web directory.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/phpMyAdmin'%(pre.split('!')[1]),
                             id     = "phpMyAdmin_Page1",
                             title  = _("phpMyAdmin Wizard"),
                             group  = _(WIZARD_GROUP_DB))

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
        self.AddPropEntry (table, _('Web Directory'),   'tmp!wizard_phpmyadmin!web_dir', NOTE_WEB_DIR, value="/phpmyadmin")
        self.AddPropEntry (table, _('Source Directory'),'tmp!wizard_phpmyadmin!sources', NOTE_SOURCES, value=guessed_src)

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
        local_src_dir = post.pop('tmp!wizard_phpmyadmin!sources')
        web_dir       = post.pop('tmp!wizard_phpmyadmin!web_dir')

        # Replacement
        php_info = wizard_php_get_info (self._cfg, self._pre)
        if not php_info:
            return self.report_error (_("Couldn't find a suitable PHP interpreter."))
        php_rule = int (php_info['rule'].split('!')[-1])

        pre_rule_plus2  = "%s!rule!%d" % (self._pre, php_rule + 2)
        pre_rule_plus1  = "%s!rule!%d" % (self._pre, php_rule + 1)

        # Add the new rules
        config = CONFIG_DIR % (locals())
        self._apply_cfg_chunk (config)
