from Form import *
from Table import *
from Module import *

class ModuleFile (Module, FormHelper):
    PROPERTIES = [
        'iocache'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'file', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'file', cfg)

    def _op_render (self):
        table = Table(2)
        self.AddTableCheckbox (table, "Use I/O cache", "%s!iocache" % (self._prefix), True)
        return str(table)

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['iocache'], post)

