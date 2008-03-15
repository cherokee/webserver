import validations

from Page import *
from Table import *
from Form import *
from consts import *

DATA_VALIDATION = [
    ("server!max_fds",                validations.is_positive_int),
    ("server!pid_file",               validations.parent_is_dir),
    ("server!sendfile_min",           validations.is_positive_int),
    ("server!sendfile_max",           validations.is_positive_int),
    ('server!panic_action',           validations.is_local_file_exists),
    ('server!listen_queue',           validations.is_positive_int),
    ('server!max_connection_reuse',   validations.is_positive_int),
    ('server!log_flush_elapse',       validations.is_positive_int),
    ('server!keepalive_max_requests', validations.is_positive_int)
]

WARNING = """
<p><b>WARNING</b>: This section contains advanced configuration
parameters. It is recommended to not change anything unless you
really know what you are doing.</p>
"""

class PageAdvanced (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'advanced', cfg)
        FormHelper.__init__ (self, 'advanced', cfg)

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', 'Advanced configuration')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_content (self):
        txt = "<h1>Advanced configuration</h1>"
        txt += self.Dialog(WARNING, 'warning')

        txt += "<h2>System tweaking</h2>"
        table = Table(2)
        self.AddTableOptions  (table, 'Thread Policy',          'server!thread_policy', THREAD_POLICY)
        self.AddTableEntry    (table, 'File descriptor number', 'server!max_fds')
        txt += self.Indent(table)

        txt += "<h2>Server tweaking</h2>"
        table = Table(2)
        self.AddTableOptions  (table, 'Polling Method',    'server!poll_method', POLL_METHODS)
        self.AddTableEntry    (table, 'Sendfile min size', 'server!sendfile_min')
        self.AddTableEntry    (table, 'Sendfile max size', 'server!sendfile_max')
        self.AddTableEntry    (table, 'Panic action',      'server!panic_action')
        self.AddTableEntry    (table, 'PID file',          'server!pid_file')
        txt += self.Indent(table)

        txt += "<h2>Server behaviour</h2>"
        table = Table(2)
        self.AddTableEntry    (table, 'Listening queue lenght', 'server!listen_queue')
        self.AddTableEntry    (table, 'Reuse connections',      'server!max_connection_reuse')
        self.AddTableEntry    (table, 'Log flush time (<i>secs</i>)','server!log_flush_elapse')
        self.AddTableEntry    (table, 'Max keepalive requests', 'server!keepalive_max_requests')
        txt += self.Indent(table)

        form = Form ("/%s" % (self._id))
        return form.Render(txt,DEFAULT_SUBMIT_VALUE)
	
    def _op_apply_changes (self, uri, post):
        self.ApplyChanges ([], post, DATA_VALIDATION) 

