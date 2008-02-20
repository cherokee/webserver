from Form import *
from Table import *
from Module import *
from validations import *

class ModuleCgiBase (Module, FormHelper):
    PROPERTIES = [
        'script_alias',
        'change_user',
        'error_handler',
        'check_file',
        'pass_req_headers'
    ]

    def __init__ (self, cfg, prefix, name, submit_url):
        FormHelper.__init__ (self, name, cfg)
        Module.__init__ (self, name, cfg, prefix, submit_url)

    def _op_render (self):
        txt   = ""
        table = Table(2)
        self.AddTableEntry    (table, "Script Alias",  "%s!script_alias" % (self._prefix))
        self.AddTableEntry    (table, "Change to UID", "%s!change_user"  % (self._prefix))
        self.AddTableCheckbox (table, "Error handler", "%s!error_handler"% (self._prefix), False)
        self.AddTableCheckbox (table, "Check file",    "%s!check_file"   % (self._prefix), True)
        self.AddTableCheckbox (table, "Pass Request",  "%s!pass_req_headers" % (self._prefix), False)
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        checkboxes = ['error_handler', 'check_file', 'pass_req_headers']
        self.ApplyChangesPrefix (self._prefix, checkboxes, post)


class ModuleCgi (ModuleCgiBase):
    def __init__ (self, cfg, prefix, submit_url):
        ModuleCgiBase.__init__ (self, cfg, prefix, 'cgi', submit_url)

    def _op_render (self):
        return ModuleCgiBase._op_render (self)

    def _op_apply_changes (self, uri, post):
        return ModuleCgiBase._op_apply_changes (self, uri, post)
