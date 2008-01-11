from Form import *
from Table import *
from Module import *

class ModuleFile (Module, FormHelper):
    def __init__ (self, cfg, prefix):
        Module.__init__ (self, 'file', cfg, prefix)
        FormHelper.__init__ (self, 'file', cfg)

    def _op_render (self):
        table = Table(2)
        self.AddTableCheckbox (table, "I/O cache", "%s!io_cache" % (self._prefix))
        return str(table)

    def _op_apply_changes (self, uri, post):
        # I/O cache checkbox
        key = '%s!io_cache' % self._prefix 
        if key in post:
            self._cfg[key] = post[key][0]
        else:
            self._cfg[key] = "0"

