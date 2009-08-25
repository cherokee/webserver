from config import *
from util import *
from Wizard import *
from configured import *

EXTENSIONS = 'mp3,ogv,flv,mov,ogg,mp4'

CONFIG = """
%(rule_pre)s!match = extensions
%(rule_pre)s!match!extensions = %(extensions)s
%(rule_pre)s!handler = streaming
%(rule_pre)s!handler!rate = 1
%(rule_pre)s!handler!rate_factor = 0.5
%(rule_pre)s!handler!rate_boost = 5
"""

class Wizard_Rules_Streaming (Wizard):
    ICON = "streaming.png"
    DESC = "Adds a rule to stream media files."

    def __init__ (self, cfg, pre):
        Wizard.__init__ (self, cfg, pre)
        self.name = "Media Streaming"

    def show (self):
        rules = self._cfg.keys('%s!rule'%(self._pre))
        for r in rules:
            if self._cfg.get_val ('%s!rule!%s!match'%(self._pre, r)) != 'extensions':
                continue
            if self._cfg.get_val ('%s!rule!%s!match!extensions'%(self._pre, r)) == EXTENSIONS:
                self.no_show = "Media streaming is already configured."                
                return False
        return True

    def _run (self, uri, post):
        _, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        if not rule_pre:
            return self.report_error ("Couldn't add a new rule.")

        extensions = EXTENSIONS
        config = CONFIG % (locals())

        self._apply_cfg_chunk (config)
