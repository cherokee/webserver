"""
phpBB wizard. Last checked with:
* Cherokee 0.99.25
* phpBB    3.0.5
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

NOTE_SOURCES  = N_("Path to the directory where the phpBB source code is located. (Example: /usr/share/phpbb)")
NOTE_WEB_DIR  = N_("Web directory where you want phpBB to be accessible. (Example: /phpbb)")
NOTE_HOST     = N_("Host name of the virtual host that is about to be created.")
ERROR_NO_SRC  = N_("Does not look like a phpBB source directory.")
ERROR_NO_WEB  = N_("A web directory must be provided.")
ERROR_NO_HOST = N_("A host name must be provided.")

CONFIG_DIR = """
%(pre_rule_plus1)s!handler = custom_error
%(pre_rule_plus1)s!handler!error = 403
%(pre_rule_plus1)s!match = request
%(pre_rule_plus1)s!match!request = ^%(web_dir)s/(config.php|common.php|cache/.*|files/.*|images/avatars/upload/.*|store/.*)

# IMPORTANT: The PHP rule comes here
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!document_root = %(local_src_dir)s
%(pre_vsrv)s!directory_index = index.php,index.html

%(pre_rule_plus1)s!handler = custom_error
%(pre_rule_plus1)s!handler!error = 403
%(pre_rule_plus1)s!match = request
%(pre_rule_plus1)s!match!request = /(config.php|common.php|cache/.*|files/.*|images/avatars/upload/.*|store/.*)

# The PHP rule comes here

%(pre_rule_minus1)s!handler = common
%(pre_rule_minus1)s!handler!iocache = 0
%(pre_rule_minus1)s!match = default
"""

SRC_PATHS = [
    "/usr/share/phpBB3",         # Debian, Fedora
    "/usr/share/phpbb",
    "/var/www/*/htdocs/phpbb",   # Gentoo
    "/srv/www/htdocs/phpbb",     # SuSE
    "/usr/local/www/data/phpbb*" # BSD
]

def is_phpbb_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    module_inc = os.path.join (path, 'includes/bbcode.php')
    if not os.path.exists (module_inc):
        raise ValueError, _(ERROR_NO_SRC)
    return path

DATA_VALIDATION = [
    ("tmp!wizard_phpbb!sources", (is_phpbb_dir, 'cfg')),
    ("tmp!wizard_phpbb!host",    (validations.is_new_host, 'cfg')),
    ("tmp!wizard_phpbb!web_dir",  validations.is_dir_formated)
]

class Wizard_VServer_phpBB (WizardPage):
    ICON = "phpbb.png"
    DESC = _("Configure a new virtual server using phpBB.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/phpBB',
                             id     = "phpBB_Page1",
                             title  = _("phpBB Wizard"),
                             group  = _(WIZARD_GROUP_MISC))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt  = '<h1>%s</h1>' % (self.title)
        guessed_src = path_find_w_default (SRC_PATHS)

        txt += '<h2>phpBB</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'),    'tmp!wizard_phpbb!host',    _(NOTE_HOST),    value="bulletin.example.com")
        self.AddPropEntry (table, _('Source Directory'), 'tmp!wizard_phpbb!sources', _(NOTE_SOURCES), value=guessed_src)
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Logging"))
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_phpbb!host',    _(ERROR_NO_HOST))
        self.Validate_NotEmpty (post, 'tmp!wizard_phpbb!sources', _(ERROR_NO_SRC))

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        local_src_dir = post.pop('tmp!wizard_phpbb!sources')
        host          = post.pop('tmp!wizard_phpbb!host')
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

        # Common static
        pre_rule_plus3  = "%s!rule!%d" % (pre_vsrv, php_rule + 3)
        self._common_add_usual_static_files (pre_rule_plus3)

        # Add the new rules
        config = CONFIG_VSERVER % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, pre_vsrv)


class Wizard_Rules_phpBB (WizardPage):
    ICON = "phpbb.png"
    DESC = "Configures phpBB inside a public web directory."

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/phpBB'%(pre.split('!')[1]),
                             id     = "phpBB_Page1",
                             title  = _("phpBB Wizard"),
                             group  = _(WIZARD_GROUP_MISC))

    def show (self):
        # Check for PHP
        php_info = wizard_php_get_info (self._cfg, self._pre)
        if not php_info:
            self.no_show = "PHP support is required."
            return False
        return True

    def _render_content (self, url_pre):
        guessed_src = path_find_w_default (SRC_PATHS)

        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'),   'tmp!wizard_phpbb!web_dir', _(NOTE_WEB_DIR), value="/bulletin")
        self.AddPropEntry (table, _('Source Directory'),'tmp!wizard_phpbb!sources', _(NOTE_SOURCES), value=guessed_src)

        txt  = '<h1>%s</h1>' % (self.title)
        txt += self.Indent(table)
        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_phpbb!web_dir', _(ERROR_NO_WEB))
        self.Validate_NotEmpty (post, 'tmp!wizard_phpbb!sources', _(ERROR_NO_SRC))

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        local_src_dir = post.pop('tmp!wizard_phpbb!sources')
        web_dir       = post.pop('tmp!wizard_phpbb!web_dir')

        # Replacement
        php_info = wizard_php_get_info (self._cfg, self._pre)
        php_rule = int (php_info['rule'].split('!')[-1])

        pre_rule_plus1  = "%s!rule!%d" % (self._pre, php_rule + 1)

        # Add the new rules
        config = CONFIG_DIR % (locals())
        self._apply_cfg_chunk (config)
