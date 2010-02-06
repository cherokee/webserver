"""
Symfony wizard. Checked with:
* Cherokee 0.99.25
* Symfony  1.2.9

* Cherokee 0.99.44
* Symfony  1.2.10, 1.3.1, 1.4.1
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

NOTE_SOURCES  = N_("Path to the directory where Symfony is installed. (Example: /usr/share/php/data/symfony)")
NOTE_DROOT    = N_("Path to the web folder of the Symfony project. (Example: /home/user/sf_project/web)")
NOTE_WEB_DIR  = N_("Web directory where you want Symfony to be accessible. (Example: /app)")
NOTE_HOST     = N_("Host name of the virtual host that is about to be created.")
ERROR_NO_SRC  = N_("Does not look like a Symfony source directory.")

CONFIG_DIR = """
%(pre_rule_plus1)s!document_root = %(local_src_dir)s/web/sf
%(pre_rule_plus1)s!encoder!gzip = 1
%(pre_rule_plus1)s!handler = file
%(pre_rule_plus1)s!handler!iocache = 1
%(pre_rule_plus1)s!match = directory
%(pre_rule_plus1)s!match!directory = %(web_dir)s/sf

# PHP rule

%(pre_rule_minus1)s!match = and
%(pre_rule_minus1)s!match!final = 1
%(pre_rule_minus1)s!match!left = directory
%(pre_rule_minus1)s!match!left!directory = %(web_dir)s
%(pre_rule_minus1)s!match!right = exists
%(pre_rule_minus1)s!match!right!iocache = 1
%(pre_rule_minus1)s!match!right!match_any = 1
%(pre_rule_minus1)s!handler = file
%(pre_rule_minus1)s!handler!iocache = 1

%(pre_rule_minus2)s!match = request
%(pre_rule_minus2)s!match!request = %(web_dir)s/(.+)
%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!show = 0
%(pre_rule_minus2)s!handler!rewrite!1!substring = %(web_dir)s/index.php
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!document_root = %(document_root)s
%(pre_vsrv)s!directory_index = index.php,index.html

%(pre_rule_plus1)s!document_root = %(local_src_dir)s/web/sf
%(pre_rule_plus1)s!encoder!gzip = 1
%(pre_rule_plus1)s!handler = file
%(pre_rule_plus1)s!handler!iocache = 1
%(pre_rule_plus1)s!match = directory
%(pre_rule_plus1)s!match!directory = /sf

# PHP rule

%(pre_rule_minus1)s!handler = file
%(pre_rule_minus1)s!match = and
%(pre_rule_minus1)s!match!right = exists
%(pre_rule_minus1)s!match!right!match_any = 1
%(pre_rule_minus1)s!match!right!match_index_files = 1
%(pre_rule_minus1)s!match!right!match_only_files = 1
%(pre_rule_minus1)s!match!left = not
%(pre_rule_minus1)s!match!left!right = request
%(pre_rule_minus1)s!match!left!right!request = ^/$

%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!regex = /.+
%(pre_rule_minus2)s!handler!rewrite!1!show = 0
%(pre_rule_minus2)s!handler!rewrite!1!substring = /index.php
%(pre_rule_minus2)s!match = default
"""

SRC_PATHS = [
    "/usr/share/php/data/symfony1.4",         # Debian, Fedora
    "/usr/share/php/data/symfony1.3",
    "/usr/share/php/data/symfony1.2",
    "/usr/share/php/data/symfony1.0",
    "/usr/share/php/data/symfony",
    "/usr/local/lib/php/data/symfony",        # Pear installation
    "/usr/share/pear/data/symfony",
    "/usr/share/pear/symfony",
]

def is_symfony_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)

    module_inc = os.path.join (path, 'bin/symfony')
    if os.path.exists (module_inc):
        return path

    module_inc = os.path.join (path, 'bin/check_configuration.php')
    if os.path.exists (module_inc):
        return path

    raise ValueError, _(ERROR_NO_SRC)

DATA_VALIDATION = [
    ("tmp!wizard_symfony!sources", (is_symfony_dir, 'cfg')),
    ("tmp!wizard_symfony!host",    (validations.is_new_host, 'cfg')),
    ("tmp!wizard_symfony!web_dir",  validations.is_dir_formated),
    ("tmp!wizard_symfony!document_root",  (validations.is_local_dir_exists, 'cfg'))
]

class Wizard_VServer_Symfony (WizardPage):
    ICON = "symfony.png"
    DESC = _("Configure a new virtual server using Symfony.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/Symfony',
                             id     = "Symfony_Page1",
                             title  = _("Symfony Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def show (self):
        return True

    def _render_content (self, url_pre):
        txt  = '<h1>%s</h1>' % (self.title)
        guessed_src = path_find_w_default (SRC_PATHS)

        txt += '<h2>Symfony</h2>'
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'),   'tmp!wizard_symfony!host',    _(NOTE_HOST),    value="app.example.com")
        self.AddPropEntry (table, _('Project Source'),  'tmp!wizard_symfony!document_root', _(NOTE_DROOT), value="/var/www")
        self.AddPropEntry (table, _('Symfony Package'), 'tmp!wizard_symfony!sources', _(NOTE_SOURCES), value=guessed_src)
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Logging"))
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_symfony!sources',       _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_symfony!document_root', _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_symfony!host',          _(ERROR_EMPTY))
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        local_src_dir = post.pop('tmp!wizard_symfony!sources')
        document_root = post.pop('tmp!wizard_symfony!document_root')
        host          = post.pop('tmp!wizard_symfony!host')
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

        # Add the new rules
        config = CONFIG_VSERVER % (locals())
        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, pre_vsrv)


class Wizard_Rules_Symfony (WizardPage):
    ICON = "symfony.png"
    DESC = _("Configures Symfony inside a public web directory.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/Symfony'%(pre.split('!')[1]),
                             id     = "Symfony_Page1",
                             title  = _("Symfony Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

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
        self.AddPropEntry (table, _('Web Directory'),   'tmp!wizard_symfony!web_dir', _(NOTE_WEB_DIR), value="/app")
        self.AddPropEntry (table, _('Project Source'),  'tmp!wizard_symfony!document_root', _(NOTE_DROOT), value="/var/www")
        self.AddPropEntry (table, _('Symfony Package'), 'tmp!wizard_symfony!sources', _(NOTE_SOURCES), value=guessed_src)

        txt  = '<h1>%s</h1>' % (self.title)
        txt += self.Indent(table)
        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self.Validate_NotEmpty (post, 'tmp!wizard_symfony!sources',       _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_symfony!document_root', _(ERROR_EMPTY))
        self.Validate_NotEmpty (post, 'tmp!wizard_symfony!web_dir',       _(ERROR_EMPTY))
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        local_src_dir = post.pop('tmp!wizard_symfony!sources')
        document_root = post.pop('tmp!wizard_symfony!document_root')
        web_dir       = post.pop('tmp!wizard_symfony!web_dir')

        # Replacement
        php_info = wizard_php_get_info (self._cfg, self._pre)
        php_rule = int (php_info['rule'].split('!')[-1])

        pre_rule_plus1  = "%s!rule!%d" % (self._pre, php_rule + 1)
        pre_rule_minus1 = "%s!rule!%d" % (self._pre, php_rule - 1)
        pre_rule_minus2 = "%s!rule!%d" % (self._pre, php_rule - 2)

        # Add the new rules
        config = CONFIG_DIR % (locals())
        self._apply_cfg_chunk (config)
