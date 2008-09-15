from Table import *
from ModuleHandler import *
from validations import *

NOTE_SCRIPT_ALIAS  = 'Path to an executable that will be run with the CGI as parameter.'
NOTE_CHANGE_USER   = 'Execute the CGI under its owner user ID.'
NOTE_ERROR_HANDLER = 'Send errors exactly as they are generated.'
NOTE_CHECK_FILE    = 'Check whether the file is in place.'
NOTE_PASS_REQ      = 'Pass all the headers to the CGI as they were received by the web server.'
NOTE_XSENDFILE     = 'Allow the use of the non-standard X-Sendfile header.'

HELPS = [
    ('modules_handlers_cgi', "CGIs")
]

class ModuleCgiBase (ModuleHandler):
    PROPERTIES = [
        'script_alias',
        'change_user',
        'error_handler',
        'check_file',
        'pass_req_headers',
        'xsendfile'
    ]

    def __init__ (self, cfg, prefix, name, submit_url):
        ModuleHandler.__init__ (self, name, cfg, prefix, submit_url)

        self.fixed_check_file  = None
        self.show_script_alias = True
        self.show_change_uid   = True

    def _op_render (self):
        txt   = "<h2>Common CGI options</h2>"

        table = TableProps()
        if self.show_script_alias:
            self.AddPropEntry (table, "Script Alias",  "%s!script_alias" % (self._prefix), NOTE_SCRIPT_ALIAS)
        if self.show_change_uid:
            self.AddPropEntry (table, "Change to UID", "%s!change_user"  % (self._prefix), NOTE_CHANGE_USER)

        self.AddPropCheck (table, "Error handler",     "%s!error_handler"% (self._prefix), False, NOTE_ERROR_HANDLER)

        if self.fixed_check_file == None:
            self.AddPropCheck (table, "Check file",    "%s!check_file"   % (self._prefix), True,  NOTE_CHECK_FILE)

        self.AddPropCheck (table, "Pass Request",      "%s!pass_req_headers" % (self._prefix), False, NOTE_PASS_REQ)
        self.AddPropCheck (table, "Allow X-Sendfile",  "%s!xsendfile"    % (self._prefix), False, NOTE_XSENDFILE)
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        checkboxes = ['error_handler', 'pass_req_headers', 'xsendfile']

        if self.fixed_check_file == None:
            checkboxes += ['check_file']
        else:
            self._cfg['%s!check_file'%(self._prefix)] = self.fixed_check_file

        self.ApplyChangesPrefix (self._prefix, checkboxes, post)

    def _util__set_fixed_check_file (self):
        # Show 'check file' when the handler isn't in a directory
        p = '!'.join(self._prefix.split('!')[:-1])
        match = self._cfg.get_val("%s!match"%(p))
        if match.lower() in ['directory', 'default']:
            self.fixed_check_file = "0"   

class ModuleCgi (ModuleCgiBase):
    def __init__ (self, cfg, prefix, submit_url):
        ModuleCgiBase.__init__ (self, cfg, prefix, 'cgi', submit_url)

    def _op_render (self):
        return ModuleCgiBase._op_render (self)

    def _op_apply_changes (self, uri, post):
        return ModuleCgiBase._op_apply_changes (self, uri, post)
