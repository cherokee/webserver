from Form import *
from Table import *
from Module import *

class ModuleServerInfo (Module, FormHelper):
    def __init__ (self, cfg, prefix):
        Module.__init__ (self, 'server_info', cfg, prefix)
        FormHelper.__init__ (self, 'server_info', cfg)

    def _op_render (self):
        table = Table(2)
        self.AddTableCheckbox (table, "Minimalist", "%s!just_about" % (self._prefix))
        return str(table)

    def _op_apply_changes (self, uri, post):
        # 'Just About' checkbox
        key = '%s!just_about' % self._prefix 
        if key in post:
            self._cfg[key] = post[key][0]
        else:
            self._cfg[key] = "0"


