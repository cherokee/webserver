import validations

from Form import *
from Table import *
from Module import *
from consts import *

DATA_VALIDATION = [
    ("new_error_url", validations.is_url_or_path)
]

REDIRECTION_TYPE = [
    ('0', 'Internal'),
    ('1', 'External')
]

TABLE_JS = """
<script type="text/javascript">
     $(document).ready(function() {
        $("#errors tr:even').addClass('alt')");
        $("table.rulestable tr:odd").addClass("odd");
     });
</script>
"""

class ModuleErrorRedir (Module, FormHelper):
    PROPERTIES = [x[0] for x in ERROR_CODES]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'error_redir', cfg)
        Module.__init__ (self, 'error_redir', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''

        # Render error list
        errors = self._cfg[self._prefix]
        if errors and errors.has_child():
            txt += '<h3>Configured error codes</h3>'
            table = Table(4,1, style='width="90%" id="errors" class="rulestable"')
            table += ('Error', 'Redirection', 'Type', '')
            for error in errors:
                js = "post_del_key('/ajax/update', '%s!%s');" % (self._prefix, error)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                show, v = self.InstanceOptions ("%s!%s!show" % (self._prefix, error), REDIRECTION_TYPE)
                table += (error, self._cfg.get_val('%s!%s!url'%(self._prefix,error)), show, link_del)
            txt += self.Indent(table)
            txt += TABLE_JS

        # New error
        txt += '<h3>Add error codes</h3>'
        table = Table(4,1, style='width="90%"')
        table += ('Error', 'Redirection', 'Type', '')

        options = EntryOptions ('new_error_code', ERROR_CODES, noautosubmit=True)
        entry   = self.InstanceEntry('new_error_url', 'text', size=30, noautosubmit=True)
        show, v = self.InstanceOptions ('new_error_show', REDIRECTION_TYPE, noautosubmit=True)
        table += (options, entry, show, SUBMIT_ADD)

        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        self.ValidateChange_SingleKey ('new_error_url', post, DATA_VALIDATION)
        if self.has_errors():
            return

        new_error = post.pop('new_error_code')
        new_url   = post.pop('new_error_url')
        new_show  = post.pop('new_error_show')

        if new_error and new_url:
            self._cfg['%s!%s!url'%(self._prefix, new_error)]  = new_url
            self._cfg['%s!%s!show'%(self._prefix, new_error)] = new_show
