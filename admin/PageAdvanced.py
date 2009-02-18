import validations

from Page import *
from Table import *
from Form import *
from consts import *
from CherokeeManagement import *

DATA_VALIDATION = [
    ("server!fdlimit",                validations.is_positive_int),
    ("server!pid_file",              (validations.parent_is_dir, 'cfg')),
    ("server!sendfile_min",           validations.is_positive_int),
    ("server!sendfile_max",           validations.is_positive_int),
    ('server!panic_action',          (validations.is_local_file_exists, 'cfg')),
    ('server!listen_queue',           validations.is_positive_int),
    ('server!max_connection_reuse',   validations.is_positive_int),
    ('server!log_flush_lapse',        validations.is_positive_int),
    ('server!keepalive_max_requests', validations.is_positive_int),
    ("server!keepalive$",             validations.is_boolean),
    ("server!thread_number",          validations.is_positive_int),
    ("server!nonces_cleanup_lapse",   validations.is_positive_int),
    ("server!iocache$",               validations.is_boolean),
    ("server!iocache!max_size",       validations.is_positive_int),
    ("server!iocache!min_file_size",  validations.is_positive_int),
    ("server!iocache!max_file_size",  validations.is_positive_int),
    ("server!iocache!lasting_stat",   validations.is_positive_int),
    ("server!iocache!lasting_mmap",   validations.is_positive_int)
]

WARNING = """
<p><b>WARNING</b>: This section contains advanced configuration
parameters. Changing things is not recommended unless you really
know what you are doing.</p>
"""

NOTE_THREAD       = 'Defines which thread policy the OS should apply to the server.'
NOTE_THREAD_NUM   = 'If empty, Cherokee will calculate a default number.'
NOTE_FD_NUM       = 'It defines how many file descriptors the server should handle. Default is the number showed by ulimit -n'
NOTE_POLLING      = 'Allows to choose the internal file descriptor polling method.'
NOTE_SENDFILE_MIN = 'Minimum size of a file to use sendfile(). Default: 32768 Bytes.'
NOTE_SENDFILE_MAX = 'Maximum size of a file to use sendfile(). Default: 2 GB.'
NOTE_PANIC_ACTION = 'Name a program that will be called if, by some reason, the server fails. Default: <em>cherokee-panic</em>.'
NOTE_PID_FILE     = 'Path of the PID file. If empty, the file will not be created.'
NOTE_LISTEN_Q     = 'Max. length of the incoming connection queue.'
NOTE_REUSE_CONNS  = 'Set the number of how many internal connections can be held for reuse by each thread. Default 20.'
NOTE_FLUSH_TIME   = 'Sets the number of seconds between log consolidations (flushes). Default: 10 seconds.'
NOTE_NONCES_TIME  = 'Time lapse (in seconds) between Nonce cache clean ups.'
NOTE_KEEPALIVE    = 'Enables the server-wide keep-alive support. It increases the performance. It is usually set on.'
NOTE_KEEPALIVE_RS = 'Maximum number of HTTP requests that can be served by each keepalive connection.'
NOTE_CHUNKED      = 'Allows the server to use Chunked encoding to try to keep Keep-Alive enabled.'
NOTE_IO_ENABLED   = 'Activate or deactivate the I/O cache globally.'
NOTE_IO_SIZE      = 'Number of pages that the cache should handle.'
NOTE_IO_MIN_SIZE  = 'Files under this size will not be cached.'
NOTE_IO_MAX_SIZE  = 'Files over this size will not be cached.'
NOTE_IO_LAST_STAT = 'How long the file information should last cached without refreshing it.'
NOTE_IO_LAST_MMAP = 'How long the file content should last cached.'

HELPS = [('config_advanced', "Advanced")]

class PageAdvanced (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'advanced', cfg, HELPS)
        FormHelper.__init__ (self, 'advanced', cfg)
        self.set_submit_url ("/%s/"%(self._id))

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', 'Advanced configuration')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_performance (self):
        polling_methods = []
        for name, desc in POLL_METHODS:
            if ((not name) or \
                cherokee_has_polling_method (name)):
                polling_methods.append((name, desc))

        table = TableProps()
        self.AddPropCheck (table, 'Keep Alive',         'server!keepalive',        True, NOTE_KEEPALIVE)
        self.AddPropEntry (table, 'Max keepalive reqs', 'server!keepalive_max_requests', NOTE_KEEPALIVE_RS)
        self.AddPropCheck (table, 'Chunked Encoding',   'server!chunked_encoding', True, NOTE_CHUNKED)
        self.AddPropOptions_Reload (table, 'Polling Method', 'server!poll_method',  polling_methods, NOTE_POLLING)
        self.AddPropEntry (table, 'Sendfile min size',  'server!sendfile_min', NOTE_SENDFILE_MIN)
        self.AddPropEntry (table, 'Sendfile max size',  'server!sendfile_max', NOTE_SENDFILE_MAX)
        return self.Indent(table)

    def _render_resources (self):
        table = TableProps()
        self.AddPropEntry (table, 'Thread Number',          'server!thread_number',        NOTE_THREAD_NUM)
        self.AddPropOptions_Reload (table, 'Thread Policy', 'server!thread_policy', THREAD_POLICY, NOTE_THREAD)
        self.AddPropEntry (table, 'File descriptors',       'server!fdlimit',              NOTE_FD_NUM)
        self.AddPropEntry (table, 'Listening queue length', 'server!listen_queue',         NOTE_LISTEN_Q)
        self.AddPropEntry (table, 'Reuse connections',      'server!max_connection_reuse', NOTE_REUSE_CONNS)
        self.AddPropEntry (table, 'Log flush time',         'server!log_flush_lapse',     NOTE_FLUSH_TIME)
        self.AddPropEntry (table, 'Nonces clean up time',   'server!nonces_cleanup_lapse',NOTE_NONCES_TIME)
        return self.Indent(table)

    def _render_iocache (self):
        table = TableProps()
        self.AddPropCheck (table, 'Status',        'server!iocache', True,         NOTE_IO_ENABLED)
        self.AddPropEntry (table, 'Max pages',     'server!iocache!max_size',      NOTE_IO_SIZE)
        self.AddPropEntry (table, 'File Min Size', 'server!iocache!min_file_size', NOTE_IO_MIN_SIZE)
        self.AddPropEntry (table, 'File Max Size', 'server!iocache!max_file_size', NOTE_IO_MAX_SIZE)
        self.AddPropEntry (table, 'Lasting: stat', 'server!iocache!lasting_stat',  NOTE_IO_LAST_STAT)
        self.AddPropEntry (table, 'Lasting: mmap', 'server!iocache!lasting_mmap',  NOTE_IO_LAST_MMAP)
        return self.Indent(table)

    def _render_special_files (self):
        table = TableProps()
        self.AddPropEntry (table, 'Panic action', 'server!panic_action', NOTE_PANIC_ACTION)
        self.AddPropEntry (table, 'PID file',     'server!pid_file',     NOTE_PID_FILE)
        return self.Indent(table)

    def _render_content (self):
        tabs  = []
        tabs += [('Connections',   self._render_performance())]
        tabs += [('Resources',     self._render_resources())]
        tabs += [('I/O cache',     self._render_iocache())]
        tabs += [('Special Files', self._render_special_files())]

        txt = "<h1>Advanced configuration</h1>"
        txt += self.Dialog(WARNING, 'warning')
        txt += self.InstanceTab (tabs)

        form = Form ("/%s" % (self._id), add_submit=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)


    def _op_apply_changes (self, uri, post):
        self.ApplyChanges (['server!keepalive', 
                            'server!chunked_encoding', 
                            'server!iocache'], 
                           post, validation = DATA_VALIDATION)

