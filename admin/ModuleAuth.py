from Form import *
from Table import *
from Module import *
from validations import *
from consts import *

class ModuleAuthBase (Module, FormHelper):
    def __init__ (self, cfg, prefix, name, submit):
        Module.__init__ (self, name, cfg, prefix, submit)
        FormHelper.__init__ (self, name, cfg)

    def _op_render (self):
        table = Table(2)
        self.AddTableOptions (table, "Methods", "%s!methods"%(self._prefix), VALIDATOR_METHODS)
        self.AddTableEntry (table, "Realm", "%s!realm"%(self._prefix))
        self.AddTableEntry (table, "Users", "%s!users"%(self._prefix))
        return str(table)

    def _op_apply_changes (self, uri, post):
        self.ApplyChanges ([], post)
