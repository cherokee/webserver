from Form import *
from Module import *

class ModuleAdmin (Module, FormHelper):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit):
        Module.__init__ (self, 'admin', cfg, prefix, submit)
        FormHelper.__init__ (self, 'admin', cfg)

    def _op_render (self):
        return ''

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, [], post)
