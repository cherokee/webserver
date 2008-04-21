import validations

from Form import *
from Table import *
from Module import *
from consts import *

DATA_VALIDATION = [
    ("new_error_url", validations.is_url_or_path)
]


class ModuleErrorRedir (Module, FormHelper):
    PROPERTIES = [x[0] for x in ERROR_CODES]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'error_redir', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'error_redir', cfg)

    def _op_render (self):
        txt = ''
        
        # Render error list
        errors = self._cfg[self._prefix]
        if errors and errors.has_child():
            txt += '<h3>Configured error codes</h3>'
            table = Table(3,1)
            table += ('Error Code', 'URL', '')
            for error in errors:
                js = "post_del_key('/ajax/update', '%s!%s');" % (self._prefix, error)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (error, self._cfg.get_val('%s!%s'%(self._prefix,error)), link_del)
            txt += str(table)

        # New error
        txt += '<h3>Add error codes</h3>'
        table = Table(3,1)
        table += ('Error', 'URL', '')

        options = EntryOptions ('new_error_code', ERROR_CODES)
        entry = self.InstanceEntry('new_error_url', 'text', size=30)
        table += (options, entry, SUBMIT_BUTTON)

        txt += str(table)
        return txt

    def _op_apply_changes (self, uri, post):
        self.ValidateChange_SingleKey ('new_error_url', post, DATA_VALIDATION)
        if self.has_errors():
            return

        new_error = post.pop('new_error_code')
        new_url   = post.pop('new_error_url')

        if new_error and new_url:
            self._cfg['%s!%s'%(self._prefix, new_error)] = new_url
