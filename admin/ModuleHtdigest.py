from Table import *
from ModuleAuth import *

class ModuleHtdigest (ModuleAuthBase):
    def __init__ (self, cfg, prefix):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'htdigest')

    def _op_render (self):
        table = Table(2)
        self.AddTableEntry (table, "Password File", "%s!passwdfile"%(self._prefix))

        txt  = ModuleAuthBase._op_render (self)
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        ModuleAuthBase._op_apply_changes (self, uri, post)
