from Table import *
from ModuleAuth import *

NOTE_SERVER      = _('LDAP server IP address.')
NOTE_PORT        = _('LDAP server port to connect to. Defaults to 389')
NOTE_BIND_DOMAIN = _('Domain sent during the LDAP authentication operation. Optional.')
NOTE_BIND_PASSWD = _('Password to authenticate in the LDAP server.')
NOTE_BASE_DOMAIN = _('Base domain for the web server authentications.')
NOTE_FILTER      = _('Object filter. It can be empty.')
NOTE_USE_TLS     = _('Enable to use secure connections between the web and LDAP servers.')
NOTE_CA_FILE     = _('CA File for the TLS connections.')

HELPS = [
    ('modules_validators_ldap', _("LDAP"))
]

class ModuleLdap (ModuleAuthBase):
    PROPERTIES = ModuleAuthBase.PROPERTIES + [
        'server', 'port',
        'bind_dn', 'base_dn',
        'filter', 'tls',
        'ca_file'
    ]

    METHODS = ['basic']

    def __init__ (self, cfg, prefix, submit):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'ldap', submit)

    def _op_render (self):
        table = TableProps()
        self.AddPropEntry (table, _("Server"), "%s!server"%(self._prefix), NOTE_SERVER)
        self.AddPropEntry (table, _("Port"), "%s!port"%(self._prefix), NOTE_PORT)
        self.AddPropEntry (table, _("Bind Domain"), "%s!bind_dn"%(self._prefix), NOTE_BIND_DOMAIN)
        self.AddPropEntry (table, _("Bind Password"), "%s!bind_pw"%(self._prefix), NOTE_BIND_PASSWD)
        self.AddPropEntry (table, _("Base Domain"), "%s!base_dn"%(self._prefix), NOTE_BIND_DOMAIN)
        self.AddPropEntry (table, _("Filter"), "%s!filter"%(self._prefix), NOTE_FILTER)
        self.AddPropCheck (table, _('Use TLS'), "%s!tls"%(self._prefix), False, NOTE_USE_TLS)
        self.AddPropEntry (table, _("CA File"), "%s!ca_file"%(self._prefix), NOTE_CA_FILE)

        txt  = ModuleAuthBase._op_render (self)
        txt += '<h2>%s</h2>' % (_('LDAP connection'))
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        # These values must be filled out
        for key, msg in [('server', _('Server')),
                         ('base_dn', _('Base Domain'))]:
            pre = '%s!%s' % (self._prefix, key)
            self.Validate_NotEmpty (post, pre, '%s can not be empty'%(msg))

        # Apply TLS
        self.ApplyChangesPrefix (self._prefix, ['tls'], post)
        post.pop('tls')

        ModuleAuthBase._op_apply_changes (self, uri, post)
