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
    ("server!ipv6",      validations.is_boolean),
    ("server!port.*",    validations.is_tcp_port),
    ("server!listen",    validations.is_ip),
    ("server!chroot",   (validations.is_local_dir_exists, 'cfg')),
]

NOTE_PORT      = 'Defines the port that the server will listen to'
NOTE_PORT_TLS  = 'Defines the port that the server will listen to for secure connections'
NOTE_IPV6      = 'Set to enable the IPv6 support. The OS must support IPv6 for this to work.'
NOTE_LISTEN    = 'IP address of the interface to bind. It is usually empty.'
NOTE_TIMEOUT   = 'Time interval until the server closes inactive connections.'
NOTE_TOKENS    = 'This option allows to choose how the server identifies itself.'
NOTE_USER      = 'Changes the effective user. User names and IDs are accepted.'
NOTE_GROUP     = 'Changes the effective group. Group names and IDs are accepted.'
NOTE_CHROOT    = 'Jail the server inside the directory. Don\'t use it as the only security measure.'

HELPS = [('config_general',    "General Configuration"),
         ('config_quickstart', "Configuration Quickstart")]

class PageGeneral (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'general', cfg, HELPS)
        FormHelper.__init__ (self, 'general', cfg)
        self.set_submit_url ("/%s/"%(self._id))

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
        self.AddPropEntry (table, 'Port TLS', 'server!port_tls', NOTE_PORT_TLS)
        self.AddPropCheck (table, 'IPv6',     'server!ipv6', True, NOTE_IPV6)
        self.AddPropEntry (table, 'Listen',   'server!listen',   NOTE_LISTEN)
        txt += self.Indent(table)

        txt += "<h2>Basic Behavior</h2>"
        table = TableProps()
        self.AddPropEntry (table,  'Timeout (<i>secs</i>)', 'server!timeout',  NOTE_TIMEOUT)
        self.AddPropOptions_Reload (table, 'Server Tokens',  'server!server_tokens', PRODUCT_TOKENS, NOTE_TOKENS)
        txt += self.Indent(table)

        txt += "<h2>Server Permissions</h2>"
        table = TableProps()
        self.AddPropEntry (table, 'User',  'server!user',    NOTE_USER)
        self.AddPropEntry (table, 'Group', 'server!group',   NOTE_GROUP)
        self.AddPropEntry (table, 'Chroot', 'server!chroot', NOTE_CHROOT)
        txt += self.Indent(table)

        form = Form ("/%s" % (self._id), add_submit=False)
        return form.Render(txt,DEFAULT_SUBMIT_VALUE)
	
    def _op_apply_changes (self, uri, post):
        self.ApplyChanges (['server!ipv6'], post, validation = DATA_VALIDATION)
                           
