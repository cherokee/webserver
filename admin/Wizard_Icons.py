from config import *
from util import *
from Wizard import *
from configured import *

CONFIG_ICONS = """
%(rule_pre_2)s!match = directory
%(rule_pre_2)s!match!directory = /icons
%(rule_pre_2)s!handler = file
%(rule_pre_2)s!handler!iocache = 1
%(rule_pre_2)s!document_root = %(droot_icons)s
%(rule_pre_2)s!encoder!gzip = 0
%(rule_pre_2)s!encoder!deflate = 0
%(rule_pre_2)s!expiration = time
%(rule_pre_2)s!expiration!time = 1h
"""

CONFIG_THEMES = """
%(rule_pre_1)s!match = directory
%(rule_pre_1)s!match!directory = /cherokee_themes
%(rule_pre_1)s!handler = file
%(rule_pre_1)s!handler!iocache = 1
%(rule_pre_1)s!document_root = %(droot_themes)s
%(rule_pre_1)s!encoder!gzip = 0
%(rule_pre_1)s!encoder!deflate = 0
%(rule_pre_1)s!expiration = time
%(rule_pre_1)s!expiration!time = 1h
"""

class Wizard_Rules_Icons (Wizard):
    ICON = "icons.png"
    DESC = _("Add the /icons and /cherokee_themes directories so Cherokee can use icons when listing directories.")

    def __init__ (self, cfg, pre):
        Wizard.__init__ (self, cfg, pre)
        self.name        = _("Add the /icons directory")
        self.have_icons  = False
        self.have_themes = False
        self.group = _(WIZARD_GROUP_TASKS)

    def _check_config (self):
        rules = self._cfg.keys('%s!rule'%(self._pre))
        for r in rules:
            if self._cfg.get_val ('%s!rule!%s!match'%(self._pre, r)) == 'directory' and \
               self._cfg.get_val ('%s!rule!%s!match!directory'%(self._pre, r)) == '/icons':
                self.have_icons = True
            if self._cfg.get_val ('%s!rule!%s!match'%(self._pre, r)) == 'directory' and \
               self._cfg.get_val ('%s!rule!%s!match!directory'%(self._pre, r)) == '/cherokee_themes':
                self.have_themes = True

        if self.have_icons and self.have_themes:
            self.no_show = _("The /icons and /cherokee_themes directories are already configured.")
            return False

        return True

    def show (self):
        return self._check_config()

    def _run (self, uri, post):
        rule_n, x = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        if not rule_n:
            return self.report_error (_("Couldn't add a new rule."))

        config_src = ''

        self._check_config()
        if not self.have_icons:
            config_src += CONFIG_ICONS
        if not self.have_themes:
            config_src += CONFIG_THEMES

        rule_pre_1   = '%s!rule!%d'%(self._pre, rule_n)
        rule_pre_2   = '%s!rule!%d'%(self._pre, rule_n+1)
        droot_icons  = CHEROKEE_ICONSDIR
        droot_themes = CHEROKEE_THEMEDIR

        config = config_src % (locals())
        self._apply_cfg_chunk (config)

