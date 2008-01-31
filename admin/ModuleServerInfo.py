from Form import *
from Table import *
from Module import *

class ModuleServerInfo (Module, FormHelper):
    def __init__ (self, cfg, prefix):
        Module.__init__ (self, 'server_info', cfg, prefix)
        FormHelper.__init__ (self, 'server_info', cfg)

    def _op_render (self):
        table = Table(2)
        self.AddTableCheckbox (table, "Just 'About'", "%s!just_about" % (self._prefix), False)
        self.AddTableCheckbox (table, "Show Connections", "%s!connection_details" % (self._prefix), False)
        return str(table)

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['just_about'], post)
