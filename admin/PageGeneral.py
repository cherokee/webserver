from Page import *
from Table import *
from Entry import *
from Form import *
from validations import *


PRODUCT_TOKENS = [
    ('',        'Default'),
    ('product', 'Product only'),
    ('minor',   'Product + Minor version'),
    ('minimal', 'Product + Minimal version'),
    ('os',      'Product + Platform'),
    ('full',    'Full Server string')
]

DATA_VALIDATION = [
    ("server!keepalive", validate_boolean),
    ("server!ipv6",      validate_boolean),
    ("server!port.*",    validate_tcp_port)
]


class PageGeneral (PageMenu):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, cfg)
        self._id  = 'general'

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', 'General configuration')
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

        # Additionaly, the checkboxes
        for checkbox in ['server!ipv6', 'server!keepalive']:
            if checkbox in post:
                self._cfg[checkbox] = post[checkbox][0]
            else:
                self._cfg[checkbox] = "0"

    def _render_content (self):
        txt = "<h2>Networking</h2>"
        table = Table(2)
        self.AddTableEntry    (table, 'Port',     'server!port')
        self.AddTableEntry    (table, 'Port TLS', 'server!port_tls')
        self.AddTableCheckbox (table, 'IPv6',     'server!ipv6')
        self.AddTableEntry    (table, 'Listen',   'server!listen')
        txt += str(table)

        txt += "<h2>Basic Behaviour</h2>"
        table = Table(2)
        self.AddTableEntry    (table, 'Timeout',        'server!timeout')
        self.AddTableCheckbox (table, 'KeepAlive',      'server!keepalive')
        self.AddTableOptions  (table, 'Server Tokens',  'server!server_tokens', PRODUCT_TOKENS)
        txt += str(table)

        txt += "<h2>Server Permissions</h2>"
        table = Table(2)
        self.AddTableEntry (table, 'User',   'server!user')
        self.AddTableEntry (table, 'User',   'server!group')
        self.AddTableEntry (table, 'Chroot', 'server!chroot')
        txt += str(table)

        txt += "<h2>System Paths</h2>"
        table = Table(2)
        self.AddTableEntry (table, 'PID file', 'server!pid_file')
        txt += str(table)

        form = Form ("/%s/update" % (self._id))
        return form.Render(txt)
