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
        self.ApplyChangesPrefix (self._prefix, ['just_about'], post)
