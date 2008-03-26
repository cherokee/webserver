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
]

NOTE_PORT      = 'Defines the port that the server will listens to'
NOTE_PORT_TLS  = 'Defines the port that the server will listens to for secure connections'
NOTE_IPV6      = 'This option enables the IPv6 support. If it is unselected the server will not use IPv6 even if the operating system supports it.'
NOTE_LISTEN    = 'IP Address to which the server will bind itself while waiting for incoming requests. '
NOTE_TIMEOUT   = 'This option specifies the connection timeout'
NOTE_KEEPALIVE = 'It enables the global keep alive support for the client requests. It increases the client performance by reusing a connection for more than one request. It is usually a good idea leave it on.'
NOTE_TOKENS    = 'This option allow to choose how the server identify itself.'
NOTE_USER      = 'Changes the effective user if the server is launched with enough privileges. User names and IDs are accepted.'
NOTE_GROUP     = 'Changes the effective group if the server is launched with enough privileges. Group names and IDs are accepted.'
NOTE_CHROOT    = 'Jail the server inside the directory. It is important to know that, even if chroot is a security mechanism, it should not be used as the only security measure.'


class PageGeneral (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'general', cfg)
        FormHelper.__init__ (self, 'general', cfg)

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', 'General configuration')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_content (self):
        txt = "<h1>General Settings</h1>"

        txt += "<h2>Networking</h2>"
        table = TableProps()
        self.AddPropEntry (table, 'Port',     'server!port',     NOTE_PORT)
        self.AddPropEntry (table, 'Port TLS', 'server!tls_port', NOTE_PORT_TLS)
        self.AddPropCheck (table, 'IPv6',     'server!ipv6', True, NOTE_IPV6)
        self.AddPropEntry (table, 'Listen',   'server!listen',   NOTE_LISTEN)
        txt += self.Indent(table)

        txt += "<h2>Basic Behaviour</h2>"
        table = TableProps()
        self.AddPropEntry (table,  'Timeout (<i>secs</i>)', 'server!timeout',  NOTE_TIMEOUT)
        self.AddPropCheck (table,  'Keep Alive',            'server!keepalive', True, NOTE_KEEPALIVE)
        self.AddPropOptions(table, 'Server Tokens',         'server!server_tokens', PRODUCT_TOKENS, NOTE_TOKENS)
        txt += self.Indent(table)

        txt += "<h2>Server Permissions</h2>"
        table = TableProps()
        self.AddPropEntry (table, 'User',  'server!user',    NOTE_USER)
        self.AddPropEntry (table, 'Group', 'server!group',   NOTE_GROUP)
        self.AddPropEntry (table, 'Chroot', 'server!chroot', NOTE_CHROOT)
        txt += self.Indent(table)

        form = Form ("/%s" % (self._id))
        return form.Render(txt,DEFAULT_SUBMIT_VALUE)
	
    def _op_apply_changes (self, uri, post):
        self.ApplyChanges (['server!ipv6', 'server!keepalive'], post, 
                           validation = DATA_VALIDATION)
