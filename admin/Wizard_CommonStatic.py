from config import *
from util import *
from Wizard import *
from configured import *

CONFIG = """
%(rule_pre)s!match = fullpath
%(rule_pre)s!handler = file
%(rule_pre)s!handler!iocache = 1
%(rule_pre)s!encoder!gzip = 0
%(rule_pre)s!encoder!deflate = 0
%(rule_pre)s!expiration = time
%(rule_pre)s!expiration!time = 1h
"""

class Wizard_Rules_CommonStatic (Wizard):
    ICON = "common_static.png"
    DESC = "Adds a rule to serve the most common static files as files."

    def __init__ (self, cfg, pre):
        Wizard.__init__ (self, cfg, pre)
        self.name = "Common Static"

    def show (self):
        rules = self._cfg.keys('%s!rule'%(self._pre))
        for r in rules:
            if self._cfg.get_val ('%s!rule!%s!match'%(self._pre, r)) == 'fullpath':
                files   = USUAL_STATIC_FILES[:]
                entries = self._cfg.keys('%s!rule!%s!match!fullpath'%(self._pre, r))
                for e in entries:
                    f = self._cfg.get_val('%s!rule!%s!match!fullpath!%s'%(self._pre, r, e))
                    try:
                        files.remove(f)
                    except ValueError:
                        pass
                if not len(files):
                    self.no_show = "Common files has been already configured."
                    return False
        return True

    def _run (self, uri, post):
        _, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        if not rule_pre:
            return self.report_error ("Couldn't add a new rule.")

        config_tmp = CONFIG
        for n in range(len(USUAL_STATIC_FILES)):
            file = USUAL_STATIC_FILES[n]
            config_tmp += "%s!match!fullpath!%s = %s\n" % ("%(rule_pre)s", n+1, file)

        droot  = CHEROKEE_ICONSDIR
        config = config_tmp % (locals())

        self._apply_cfg_chunk (config)
