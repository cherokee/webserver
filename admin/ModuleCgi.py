from Form import *
from Table import *
from Module import *
from validations import *

class ModuleCgiBase (Module, FormHelper):
    def __init__ (self, cfg, prefix):
        Module.__init__ (self, 'cgi', cfg, prefix)
        FormHelper.__init__ (self, 'cgi', cfg)

    def _op_render (self):
        table = Table(2)
        self.AddTableEntry    (table, "Script Alias",  "%s!script_alias" % (self._prefix))
        self.AddTableEntry    (table, "Change to UID", "%s!change_user" % (self._prefix))
        self.AddTableCheckbox (table, "Error handler", "%s!error_handler" % (self._prefix))
        self.AddTableCheckbox (table, "Check file",    "%s!check_file" % (self._prefix))
        self.AddTableCheckbox (table, "Pass Request Headers", "%s!pass_req_headers" % (self._prefix))

        return str(table)

    def _op_apply_changes (self, uri, post):
        checkboxes = ['error_handler', 'check_file', 'pass_req_headers']
        self.ApplyChangesPrefix (self._prefix, checkboxes, post)


class ModuleCgi (ModuleCgiBase):
    def __init__ (self, cfg, prefix):
        ModuleCgiBase.__init__ (self, cfg, prefix)

    def _op_render (self):
        txt = '<h2>Base</h2>'
        txt += ModuleCgiBase._op_render (self)

    def _op_apply_changes (self, uri, post):
        return ModuleCgiBase._op_apply_changes (uri, post)
