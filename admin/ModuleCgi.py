import validations 

from Table import *
from ModuleHandler import *

NOTE_SCRIPT_ALIAS  = 'Path to an executable that will be run with the CGI as parameter.'
NOTE_CHANGE_USER   = 'Execute the CGI under its file owner user ID.'
NOTE_ERROR_HANDLER = 'Send errors exactly as they are generated.'
NOTE_CHECK_FILE    = 'Check whether the file is in place.'
NOTE_PASS_REQ      = 'Forward all the client headers to the CGI encoded as HTTP_*. headers.'
NOTE_XSENDFILE     = 'Allow the use of the non-standard X-Sendfile header.'

DATA_VALIDATION = [
    ('vserver!.+?!rule!.+?!handler!script_alias', validations.is_path),
]

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
        'xsendfile',
        'env'
    ]

    def __init__ (self, cfg, prefix, name, submit_url):
        ModuleHandler.__init__ (self, name, cfg, prefix, submit_url)

        self.show_script_alias = True
        self.show_change_uid   = True

    def _op_render (self):
        txt   = "<h2>Common CGI options</h2>"

        table = TableProps()
        if self.show_script_alias:
            self.AddPropEntry (table, "Script Alias",  "%s!script_alias" % (self._prefix), NOTE_SCRIPT_ALIAS)
        if self.show_change_uid:
            self.AddPropCheck (table, "Change UID", "%s!change_user"%(self._prefix), False, NOTE_CHANGE_USER)

        self.AddPropCheck (table, "Error handler",     "%s!error_handler"% (self._prefix), False, NOTE_ERROR_HANDLER)

        self.AddPropCheck (table, "Check file",           "%s!check_file"   % (self._prefix), True,  NOTE_CHECK_FILE)
        self.AddPropCheck (table, "Pass Request Headers", "%s!pass_req_headers" % (self._prefix), True,  NOTE_PASS_REQ)
        self.AddPropCheck (table, "Allow X-Sendfile",     "%s!xsendfile" % (self._prefix),        False, NOTE_XSENDFILE)
        txt += self.Indent(table)

        txt1 = '<h2>Custom environment variables</h2>'
        envs = self._cfg.keys('%s!env'%(self._prefix))
        if envs:
            table = Table(3, title_left=1, style='width="90%"')

            for env in envs:
                pre = '%s!env!%s'%(self._prefix,env)
                val = self.InstanceEntry(pre, 'text', size=25)
                js = "post_del_key('/ajax/update', '%s');"%(pre)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (env, val, link_del)

            txt1 += self.Indent(table)

        txt1 += '<h3>Add new custom environment variable</h3>';
        name  = self.InstanceEntry('new_custom_env_name',  'text', size=25, noautosubmit=True)
        value = self.InstanceEntry('new_custom_env_value', 'text', size=25, noautosubmit=True)

        table = Table(3, 1, style='width="90%"')
        table += ('Name', 'Value', '')
        table += (name, value, SUBMIT_ADD)
        txt1 += self.Indent(table)
        txt += txt1

        return txt

    def _op_apply_changes (self, uri, post):
        new_name = post.pop('new_custom_env_name')
        new_value = post.pop('new_custom_env_value')

        if new_name and new_value:
            self._cfg['%s!env!%s'%(self._prefix, new_name)] = new_value

        checkboxes = ['error_handler', 'pass_req_headers', 'xsendfile',
                      'change_user', 'check_file']

        self.ApplyChangesPrefix (self._prefix, checkboxes, post, DATA_VALIDATION)

class ModuleCgi (ModuleCgiBase):
    def __init__ (self, cfg, prefix, submit_url):
        ModuleCgiBase.__init__ (self, cfg, prefix, 'cgi', submit_url)

    def _op_render (self):
        return ModuleCgiBase._op_render (self)

    def _op_apply_changes (self, uri, post):
        return ModuleCgiBase._op_apply_changes (self, uri, post)
