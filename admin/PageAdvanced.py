from Page import *
from Table import *
from Entry import *
from Form import *
from validations import *

THREAD_POLICY = [
    ('',      'Default'),
    ('fifo',  'FIFO'),
    ('rr',    'Round-robin'),
    ('other', 'Dynamic')
]

POLL_METHODS = [
    ('',       'Automatic'),
    ('epoll',  'epoll() - Linux >= 2.6'),
    ('kqueue', 'kqueue() - BSD, OS X'),
    ('ports',  'Solaris ports - >= 10'),
    ('poll',   'poll()'),
    ('select', 'select()'),
    ('win32',  'Win32')
]

DATA_VALIDATION = []


class PageAdvanced (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'advanced', cfg)
        FormHelper.__init__ (self, 'advanced', cfg)

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', 'Advanced configuration')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _op_handler (self, uri, post):
        self._op_apply_changes (post)
        return "/%s" % (self._id)

    def _op_apply_changes (self, post):
        self.ValidateChanges (post, DATA_VALIDATION)

        # Modify posted entries
        for confkey in post:
            self._cfg[confkey] = post[confkey][0]

    def _render_content (self):
        txt = "<h2>System tweaking</h2>"
        table = Table(2)
        self.AddTableOptions  (table, 'Thread Policy',  'server!thread_policy', THREAD_POLICY)
        self.AddTableEntry    (table, 'File descriptor number', 'server!max_fds')
        txt += str(table)

        txt += "<h2>Server tweaking</h2>"
        table = Table(2)
        self.AddTableOptions  (table, 'Polling Method', 'server!poll_method', POLL_METHODS)
        self.AddTableEntry    (table, 'Sendfile min size', 'server!sendfile_min')
        self.AddTableEntry    (table, 'Sendfile max size', 'server!sendfile_max')
        self.AddTableEntry    (table, 'Panic action', 'server!panic_action')
        txt += str(table)

        txt += "<h2>Server behaviour</h2>"
        table = Table(2)
        self.AddTableEntry    (table, 'Listening queue lenght', 'server!listen_queue')
        self.AddTableEntry    (table, 'Reuse connections', 'server!max_connection_reuse')
        self.AddTableEntry    (table, 'Log flush time', 'server!log_flush_elapse')
        self.AddTableEntry    (table, 'Max keepalive requests', 'server!keepalive_max_requests')
        txt += str(table)

        form = Form ("/%s/update" % (self._id))
        return form.Render(txt)



