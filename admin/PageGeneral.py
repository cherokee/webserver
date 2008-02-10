import validations

from Page import *
from Table import *
from Entry import *
from Form import *


PRODUCT_TOKENS = [
    ('',        'Default'),
    ('product', 'Product only'),
    ('minor',   'Product + Minor version'),
    ('minimal', 'Product + Minimal version'),
    ('os',      'Product + Platform'),
    ('full',    'Full Server string')
]

DATA_VALIDATION = [
    ("server!keepalive", validations.is_boolean),
    ("server!ipv6",      validations.is_boolean),
    ("server!port.*",    validations.is_tcp_port),
    ("server!listen",    validations.is_ip),
    ("server!chroot",    validations.is_local_dir_exists),
    ("server!pid_file",  validations.parent_is_dir),
]

class PageGeneral (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'general', cfg)
        FormHelper.__init__ (self, 'general', cfg)

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', 'General configuration')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _op_handler (self, uri, post):
        self._op_apply_changes (post)
        if self.has_errors():
            return self._op_render ()

        return "/%s" % (self._id)

    def _render_content (self):
        txt = "<h2>Networking</h2>"
        table = Table(2)
        self.AddTableEntry    (table, 'Port',     'server!port')
        self.AddTableEntry    (table, 'Port TLS', 'server!port_tls')
        self.AddTableCheckbox (table, 'IPv6',     'server!ipv6', True)
        self.AddTableEntry    (table, 'Listen',   'server!listen')
        txt += str(table)

        txt += "<h2>Basic Behaviour</h2>"
        table = Table(2)
        self.AddTableEntry    (table, 'Timeout',        'server!timeout')
        self.AddTableCheckbox (table, 'KeepAlive',      'server!keepalive', True)
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

    def _op_apply_changes (self, post):
        self.ApplyChanges (['server!ipv6', 'server!keepalive'], post, 
                           validation = DATA_VALIDATION)
