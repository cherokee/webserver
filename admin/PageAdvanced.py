import validations

from Page import *
from Table import *
from Form import *
from consts import *
from CherokeeManagement import *

# For gettext
N_ = lambda x: x

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
    ("server!iocache!lasting_mmap",   validations.is_positive_int),
    ("server!tls!dh_param512",       (validations.is_local_file_exists, 'cfg')),
    ("server!tls!dh_param1024",      (validations.is_local_file_exists, 'cfg')),
    ("server!tls!dh_param2048",      (validations.is_local_file_exists, 'cfg')),
    ("server!tls!dh_param4096",      (validations.is_local_file_exists, 'cfg'))
]

WARNING = N_('<p><b>WARNING</b>: This section contains advanced configuration parameters. Changing things is not recommended unless you really know what you are doing.</p>')

NOTE_THREAD       = N_('Defines which thread policy the OS should apply to the server.')
NOTE_THREAD_NUM   = N_('If empty, Cherokee will calculate a default number.')
NOTE_FD_NUM       = N_('It defines how many file descriptors the server should handle. Default is the number showed by ulimit -n')
NOTE_POLLING      = N_('Allows to choose the internal file descriptor polling method.')
NOTE_SENDFILE_MIN = N_('Minimum size of a file to use sendfile(). Default: 32768 Bytes.')
NOTE_SENDFILE_MAX = N_('Maximum size of a file to use sendfile(). Default: 2 GB.')
NOTE_PANIC_ACTION = N_('Name a program that will be called if, by some reason, the server fails. Default: <em>cherokee-panic</em>.')
NOTE_PID_FILE     = N_('Path of the PID file. If empty, the file will not be created.')
NOTE_LISTEN_Q     = N_('Max. length of the incoming connection queue.')
NOTE_REUSE_CONNS  = N_('Set the number of how many internal connections can be held for reuse by each thread. Default 20.')
NOTE_FLUSH_TIME   = N_('Sets the number of seconds between log consolidations (flushes). Default: 10 seconds.')
NOTE_NONCES_TIME  = N_('Time lapse (in seconds) between Nonce cache clean ups.')
NOTE_KEEPALIVE    = N_('Enables the server-wide keep-alive support. It increases the performance. It is usually set on.')
NOTE_KEEPALIVE_RS = N_('Maximum number of HTTP requests that can be served by each keepalive connection.')
NOTE_CHUNKED      = N_('Allows the server to use Chunked encoding to try to keep Keep-Alive enabled.')
NOTE_IO_ENABLED   = N_('Activate or deactivate the I/O cache globally.')
NOTE_IO_SIZE      = N_('Number of pages that the cache should handle.')
NOTE_IO_MIN_SIZE  = N_('Files under this size will not be cached.')
NOTE_IO_MAX_SIZE  = N_('Files over this size will not be cached.')
NOTE_IO_LAST_STAT = N_('How long the file information should last cached without refreshing it.')
NOTE_IO_LAST_MMAP = N_('How long the file content should last cached.')
NOTE_DH512        = N_('Path to a Diffie Hellman (DH) parameters PEM file: 512 bits.')
NOTE_DH1024       = N_('Path to a Diffie Hellman (DH) parameters PEM file: 1024 bits.')
NOTE_DH2048       = N_('Path to a Diffie Hellman (DH) parameters PEM file: 2048 bits.')
NOTE_DH4096       = N_('Path to a Diffie Hellman (DH) parameters PEM file: 4096 bits.')

HELPS = [('config_advanced', N_('Advanced'))]

class PageAdvanced (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'advanced', cfg, HELPS)
        FormHelper.__init__ (self, 'advanced', cfg)
        self.set_submit_url ("/%s/"%(self._id))

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', _('Advanced configuration'))
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_performance (self):
        polling_methods = []
        for name, desc in POLL_METHODS:
            if ((not name) or \
                cherokee_has_polling_method (name)):
                polling_methods.append((name, _(desc)))

        table = TableProps()
        self.AddPropCheck (table, _('Keep Alive'),         'server!keepalive',        True, _(NOTE_KEEPALIVE))
        self.AddPropEntry (table, _('Max keepalive reqs'), 'server!keepalive_max_requests', _(NOTE_KEEPALIVE_RS))
        self.AddPropCheck (table, _('Chunked Encoding'),   'server!chunked_encoding', True, _(NOTE_CHUNKED))
        self.AddPropOptions_Reload (table, _('Polling Method'), 'server!poll_method',  polling_methods, _(NOTE_POLLING))
        self.AddPropEntry (table, _('Sendfile min size'),  'server!sendfile_min', _(NOTE_SENDFILE_MIN))
        self.AddPropEntry (table, _('Sendfile max size'),  'server!sendfile_max', _(NOTE_SENDFILE_MAX))
        return self.Indent(table)

    def _render_resources (self):
        table = TableProps()
        self.AddPropEntry (table, _('Thread Number'),          'server!thread_number',        _(NOTE_THREAD_NUM))
        self.AddPropOptions_Reload (table, _('Thread Policy'), 'server!thread_policy', THREAD_POLICY, _(NOTE_THREAD))
        self.AddPropEntry (table, _('File descriptors'),       'server!fdlimit',              _(NOTE_FD_NUM))
        self.AddPropEntry (table, _('Listening queue length'), 'server!listen_queue',         _(NOTE_LISTEN_Q))
        self.AddPropEntry (table, _('Reuse connections'),      'server!max_connection_reuse', _(NOTE_REUSE_CONNS))
        self.AddPropEntry (table, _('Log flush time'),         'server!log_flush_lapse',     _(NOTE_FLUSH_TIME))
        self.AddPropEntry (table, _('Nonces clean up time'),   'server!nonces_cleanup_lapse',_(NOTE_NONCES_TIME))
        return self.Indent(table)

    def _render_iocache (self):
        table = TableProps()
        self.AddPropCheck (table, _('Status'),        'server!iocache', True,         _(NOTE_IO_ENABLED))
        self.AddPropEntry (table, _('Max pages'),     'server!iocache!max_size',      _(NOTE_IO_SIZE))
        self.AddPropEntry (table, _('File Min Size'), 'server!iocache!min_file_size', _(NOTE_IO_MIN_SIZE))
        self.AddPropEntry (table, _('File Max Size'), 'server!iocache!max_file_size', _(NOTE_IO_MAX_SIZE))
        self.AddPropEntry (table, _('Lasting: stat'), 'server!iocache!lasting_stat',  _(NOTE_IO_LAST_STAT))
        self.AddPropEntry (table, _('Lasting: mmap'), 'server!iocache!lasting_mmap',  _(NOTE_IO_LAST_MMAP))
        return self.Indent(table)

    def _render_special_files (self):
        table = TableProps()
        self.AddPropEntry (table, _('Panic action'), 'server!panic_action', _(NOTE_PANIC_ACTION))
        self.AddPropEntry (table, _('PID file'),     'server!pid_file',     _(NOTE_PID_FILE))
        return self.Indent(table)

    def _render_tls (self):
	table = TableProps()
        self.AddPropEntry (table, _('DH parameters: 512 bits'), 'server!tls!dh_param512', _(NOTE_DH512))
        self.AddPropEntry (table, _('DH parameters: 1024 bits'), 'server!tls!dh_param1024', _(NOTE_DH1024))
        self.AddPropEntry (table, _('DH parameters: 2048 bits'), 'server!tls!dh_param2048', _(NOTE_DH2048))
        self.AddPropEntry (table, _('DH parameters: 4096 bits'), 'server!tls!dh_param4096', _(NOTE_DH4096))
        return self.Indent(table)

    def _render_content (self):
        tabs  = []
        tabs += [(_('Connections'),   self._render_performance())]
        tabs += [(_('Resources'),     self._render_resources())]
        tabs += [(_('I/O cache'),     self._render_iocache())]
        tabs += [(_('Special Files'), self._render_special_files())]
        tabs += [(_('TLS'),           self._render_tls())]

        txt = "<h1>%s</h1>" % (_('Advanced configuration'))
        txt += self.Dialog(WARNING, 'warning')
        txt += self.InstanceTab (tabs)

        form = Form ("/%s" % (self._id), add_submit=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)


    def _op_apply_changes (self, uri, post):
        self.ApplyChanges (['server!keepalive', 
                            'server!chunked_encoding', 
                            'server!iocache'], 
                           post, validation = DATA_VALIDATION)

