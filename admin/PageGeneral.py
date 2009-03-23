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
    ("server!ipv6",              validations.is_boolean),
    ("server!timeout",           validations.is_number),
    ("server!bind!.*!port",      validations.is_tcp_port),
    ("server!bind!.*!interface", validations.is_ip),
    ("server!bind!.*!tls",       validations.is_boolean),
    ("server!chroot",           (validations.is_local_dir_exists, 'cfg')),
]

NOTE_ADD_PORT  = 'Defines a port that the server will listen to'
NOTE_IPV6      = 'Set to enable the IPv6 support. The OS must support IPv6 for this to work.'
NOTE_LISTEN    = 'IP address of the interface to bind. It is usually empty.'
NOTE_TIMEOUT   = 'Time interval until the server closes inactive connections.'
NOTE_TOKENS    = 'This option allows to choose how the server identifies itself.'
NOTE_USER      = 'Changes the effective user. User names and IDs are accepted.'
NOTE_GROUP     = 'Changes the effective group. Group names and IDs are accepted.'
NOTE_CHROOT    = 'Jail the server inside the directory. Don\'t use it as the only security measure.'
NOTE_TLS       = 'Which, if any, should be the TLS/SSL backend.'

HELPS = [('config_general',    "General Configuration"),
         ('config_quickstart', "Configuration Quickstart")]

class PageGeneral (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'general', cfg, HELPS)
        FormHelper.__init__ (self, 'general', cfg)
        self.set_submit_url ("/%s/"%(self._id))

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', 'General Configuration')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_content (self):
        txt = "<h1>General Settings</h1>"

        tabs  = []
        tabs += [('Network',            self._render_network())]
        tabs += [('Ports to listen',    self._render_ports())]
        tabs += [('Server Permissions', self._render_permissions())]

        txt += self.InstanceTab (tabs)
        form = Form ("/%s" % (self._id), add_submit=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _render_permissions (self):
        txt = "<h2>Execution Permissions</h2>"
        table = TableProps()
        self.AddPropEntry (table, 'User',   'server!user',   NOTE_USER)
        self.AddPropEntry (table, 'Group',  'server!group',  NOTE_GROUP)
        self.AddPropEntry (table, 'Chroot', 'server!chroot', NOTE_CHROOT)
        txt += self.Indent(table)
        return txt

    def _render_network (self):
        txt = "<h2>Support</h2>"
        table = TableProps()
        self.AddPropCheck (table, 'IPv6',     'server!ipv6', True, NOTE_IPV6)
        self.AddPropOptions_Reload (table, 'SSL/TLS back-end','server!tls',modules_available(CRYPTORS), NOTE_TLS)
        txt += self.Indent(table)

        txt += "<h2>Network behavior</h2>"
        table = TableProps()
        self.AddPropEntry (table,  'Timeout (<i>secs</i>)', 'server!timeout',       NOTE_TIMEOUT)
        self.AddPropOptions_Reload (table, 'Server Tokens', 'server!server_tokens', PRODUCT_TOKENS, NOTE_TOKENS)
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        checkboxes = ['server!ipv6']
        # TLS checkboxes
        checkboxes += ['server!bind!%s!tls' % (cb) for cb in self._cfg.keys('server!bind')]
        self.ApplyChanges (checkboxes, post, validation = DATA_VALIDATION)

    def _render_ports (self):
        txt = ''

        # Is it empty?
        binds = [int(x) for x in self._cfg.keys('server!bind')]
        binds.sort()
        if not binds:
            self._cfg['server!bind!1!port'] = '80'
            next = '2'
        else:
            next = str(binds[-1] + 1)

        has_tls = self._cfg.get_val('server!tls')

        # List ports
        table = Table(4, 1, style='width="90%"')
        table += ('Port', 'Bind to', 'TLS', '')

        for k in self._cfg.keys('server!bind'):
            pre = 'server!bind!%s'%(k)

            port   = self.InstanceEntry ("%s!port"%(pre),      'text', size=8)
            listen = self.InstanceEntry ("%s!interface"%(pre), 'text', size=45)
            tls    = self.InstanceCheckbox ('%s!tls'%(pre), False, quiet=True, disabled=not has_tls)

            js = "post_del_key('/ajax/update', '%s');"%(pre)
            link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                
            table += (port, listen, tls, link_del)

        txt = "<h2>Listening to ports</h2>"
        txt += self.Indent(table)
        
        # Add new port
        pre    = 'server!bind!%s!port'%(next)
        table = TableProps()
        self.AddPropEntry (table,  'Add new port', pre,  NOTE_ADD_PORT)

        txt += "<br />"
        txt += str(table)
        return txt
