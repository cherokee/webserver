from Form import *
from Table import *
from Module import *
from consts import *

NOTE_ERRORS = 'HTTP Error that you be used to reply the request.'

class ModuleCustomError (Module, FormHelper):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'custom_error', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'custom_error', cfg)

    def _op_render (self):
        table = TableProps()
        self.AddPropOptions (table, "HTTP Error", "%s!error" % (self._prefix), ERROR_CODES, NOTE_ERRORS)
        return str(table)

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, [], post)

