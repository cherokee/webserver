import validations

from Page import *
from Table import *
from Entry import *
from Form import *

# For gettext
N_ = lambda x: x

PRODUCT_TOKENS = [
    ('',        N_('Default')),
    ('product', N_('Product only')),
    ('minor',   N_('Product + Minor version')),
    ('minimal', N_('Product + Minimal version')),
    ('os',      N_('Product + Platform')),
    ('full',    N_('Full Server string'))
]

DATA_VALIDATION = [
    ("server!ipv6",              validations.is_boolean),
    ("server!timeout",           validations.is_number),
    ("server!bind!.*!port",      validations.is_tcp_port),
    ("server!bind!.*!interface", validations.is_ip),
    ("server!bind!.*!tls",       validations.is_boolean),
    ("server!chroot",           (validations.is_local_dir_exists, 'cfg')),
]

NOTE_ADD_PORT  = N_('Defines a port that the server will listen to')
NOTE_IPV6      = N_('Set to enable the IPv6 support. The OS must support IPv6 for this to work.')
NOTE_LISTEN    = N_('IP address of the interface to bind. It is usually empty.')
NOTE_TIMEOUT   = N_('Time interval until the server closes inactive connections.')
NOTE_TOKENS    = N_('This option allows to choose how the server identifies itself.')
NOTE_USER      = N_('Changes the effective user. User names and IDs are accepted.')
NOTE_GROUP     = N_('Changes the effective group. Group names and IDs are accepted.')
NOTE_CHROOT    = N_('Jail the server inside the directory. Don\'t use it as the only security measure.')
NOTE_TLS       = N_('Which, if any, should be the TLS/SSL backend.')

HELPS = [('config_general',    N_("General Configuration")),
         ('config_quickstart', N_("Configuration Quickstart"))]

class PageGeneral (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'general', cfg, HELPS)
        FormHelper.__init__ (self, 'general', cfg)
        self.set_submit_url ("/%s/"%(self._id))

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', _('General Configuration'))
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_content (self):
        txt = "<h1>%s</h1>" % (_('General Settings'))

        tabs  = []
        tabs += [(_('Network'),            self._render_network())]
        tabs += [(_('Ports to listen'),    self._render_ports())]
        tabs += [(_('Server Permissions'), self._render_permissions())]

        txt += self.InstanceTab (tabs)
        form = Form ("/%s" % (self._id), add_submit=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _render_permissions (self):
        txt = "<h2>%s</h2>" % (_('Execution Permissions'))
        table = TableProps()
        self.AddPropEntry (table, _('User'),   'server!user',   NOTE_USER)
        self.AddPropEntry (table, _('Group'),  'server!group',  NOTE_GROUP)
        self.AddPropEntry (table, _('Chroot'), 'server!chroot', NOTE_CHROOT)
        txt += self.Indent(table)
        return txt

    def _render_network (self):
        txt = "<h2>%s</h2>" % (_('Support'))
        table = TableProps()
        self.AddPropCheck (table, 'IPv6',     'server!ipv6', True, NOTE_IPV6)
        self.AddPropOptions_Reload (table, _('SSL/TLS back-end'),'server!tls',modules_available(CRYPTORS), NOTE_TLS)
        txt += self.Indent(table)

        txt += "<h2>%s</h2>" % (_('Network behavior'))
        table = TableProps()
        self.AddPropEntry (table,  _('Timeout (<i>secs</i>)'), 'server!timeout',       NOTE_TIMEOUT)
        self.AddPropOptions_Reload (table, _('Server Tokens'), 'server!server_tokens', PRODUCT_TOKENS, NOTE_TOKENS)
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
        table += (_('Port'), _('Bind to'), _('TLS'), '')

        for k in self._cfg.keys('server!bind'):
            pre = 'server!bind!%s'%(k)

            port   = self.InstanceEntry ("%s!port"%(pre),      'text', size=8)
            listen = self.InstanceEntry ("%s!interface"%(pre), 'text', size=45)
            tls    = self.InstanceCheckbox ('%s!tls'%(pre), False, quiet=True, disabled=not has_tls)

            js = "post_del_key('/ajax/update', '%s');"%(pre)
            link_del = self.InstanceImage ("bin.png", _("Delete"), border="0", onClick=js)

            table += (port, listen, tls, link_del)

        txt = "<h2>%s</h2>" % (_('Listening to ports'))
        txt += self.Indent(table)

        # Add new port
        pre    = 'server!bind!%s!port'%(next)
        table = TableProps()
        self.AddPropEntry (table,  _('Add new port'), pre,  NOTE_ADD_PORT)

        txt += "<br />"
        txt += str(table)
        return txt
