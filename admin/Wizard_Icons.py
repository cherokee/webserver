from config import *
from util import *
from Wizard import *
from configured import *

CONFIG = """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = /icons
%(rule_pre)s!handler = file
%(rule_pre)s!handler!iocache = 1
%(rule_pre)s!document_root = %(droot)s
%(rule_pre)s!encoder!gzip = 0
%(rule_pre)s!encoder!deflate = 0
%(rule_pre)s!expiration = time
%(rule_pre)s!expiration!time = 1h
"""

class Wizard_Rules_Icons (Wizard):
    ICON = "icons.png"
    DESC = "Add the /icons directory so Cherokee can use icons when listing directories."

    def __init__ (self, cfg, pre):
        Wizard.__init__ (self, cfg, pre)
        self.name   = "Add the /icons directory"

    def show (self):
        rules = self._cfg.keys('%s!rule'%(self._pre))
        for r in rules:
            if self._cfg.get_val ('%s!rule!%s!match'%(self._pre, r)) == 'directory' and \
               self._cfg.get_val ('%s!rule!%s!match!directory'%(self._pre, r)) == '/icons':
                self.no_show = "A /icons directory is already configured."
                return False
        return True

    def _run (self, uri, post):
        _, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        if not rule_pre:
            return self.report_error ("Couldn't add a new rule.")

        droot  = CHEROKEE_ICONSDIR
        config = CONFIG % (locals())

        self._apply_cfg_chunk (config)
