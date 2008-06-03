from Form import *
from Table import *
from Module import *
from validations import *

NOTE_SCRIPT_ALIAS  = 'Path to an executable that will be run with the CGI as parameter.'
NOTE_CHANGE_USER   = 'Execute the CGI under its owner user ID.'
NOTE_ERROR_HANDLER = 'Send errors exactly as they are generated.'
NOTE_CHECK_FILE    = 'Check whether the file is in place.'
NOTE_PASS_REQ      = 'Pass all the headers to the CGI as they were received by the web server.'
NOTE_XSENDFILE     = 'Allow the use of the non-standard X-Sendfile header.'

class ModuleCgiBase (Module, FormHelper):
    PROPERTIES = [
        'script_alias',
        'change_user',
        'error_handler',
        'check_file',
        'pass_req_headers',
        'xsendfile'
    ]

    def __init__ (self, cfg, prefix, name, submit_url):
        FormHelper.__init__ (self, name, cfg)
        Module.__init__ (self, name, cfg, prefix, submit_url)

    def _op_render (self):
        txt   = "<h2>Common CGI options</h2>"

        table = TableProps()
        self.AddPropEntry (table, "Script Alias",     "%s!script_alias" % (self._prefix), NOTE_SCRIPT_ALIAS)
        self.AddPropEntry (table, "Change to UID",    "%s!change_user"  % (self._prefix), NOTE_CHANGE_USER)
        self.AddPropCheck (table, "Error handler",    "%s!error_handler"% (self._prefix), False, NOTE_ERROR_HANDLER)
        self.AddPropCheck (table, "Check file",       "%s!check_file"   % (self._prefix), True,  NOTE_CHECK_FILE)
        self.AddPropCheck (table, "Pass Request",     "%s!pass_req_headers" % (self._prefix), False, NOTE_PASS_REQ)
        self.AddPropCheck (table, "Allow X-Sendfile", "%s!xsendfile"    % (self._prefix), False, NOTE_XSENDFILE)
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        checkboxes = ['error_handler', 'check_file', 'pass_req_headers', 'xsendfile']
        self.ApplyChangesPrefix (self._prefix, checkboxes, post)


class ModuleCgi (ModuleCgiBase):
    def __init__ (self, cfg, prefix, submit_url):
        ModuleCgiBase.__init__ (self, cfg, prefix, 'cgi', submit_url)

    def _op_render (self):
        return ModuleCgiBase._op_render (self)

    def _op_apply_changes (self, uri, post):
        return ModuleCgiBase._op_apply_changes (self, uri, post)
