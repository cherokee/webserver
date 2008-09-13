from Table import *
from ModuleAuth import *

NOTE_SERVER      = 'LDAP server IP address.'
NOTE_PORT        = 'LDAP server port to connect to. Defaults to 389'
NOTE_BIND_DOMAIN = 'Domain sent during the LDAP authentication operation. Optional.'
NOTE_BIND_PASSWD = 'Password to authenticate in the LDAP server.'
NOTE_BASE_DOMAIN = 'Base domain for the web server authentications.'
NOTE_FILTER      = 'Object filter. It can be empty.'
NOTE_USE_TLS     = 'Enable to use secure connections between the web and LDAP servers.'
NOTE_CA_FILE     = 'CA File for the TLS connections.'

HELPS = [
    ('modules_validators_ldap', "LDAP")
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
        self.AddPropEntry (table, "Server", "%s!server"%(self._prefix), NOTE_SERVER)
        self.AddPropEntry (table, "Port", "%s!port"%(self._prefix), NOTE_PORT)
        self.AddPropEntry (table, "Bind Domain", "%s!bind_dn"%(self._prefix), NOTE_BIND_DOMAIN)
        self.AddPropEntry (table, "Bind Password", "%s!bind_pw"%(self._prefix), NOTE_BIND_PASSWD)
        self.AddPropEntry (table, "Base Domain", "%s!base_dn"%(self._prefix), NOTE_BIND_DOMAIN)
        self.AddPropEntry (table, "Filter", "%s!filter"%(self._prefix), NOTE_FILTER)
        self.AddPropCheck (table, 'Use TLS', "%s!tls"%(self._prefix), False, NOTE_USE_TLS)
        self.AddPropEntry (table, "CA File", "%s!ca_file"%(self._prefix), NOTE_CA_FILE)

        txt  = ModuleAuthBase._op_render (self)
        txt += '<h2>LDAP connection</h2>'
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        # These values must be filled out
        for key, msg in [('server', 'Server'),
                         ('base_dn', 'Base Domain')]:
            pre = '%s!%s' % (self._prefix, key)
            self.Validate_NotEmpty (post, pre, '%s can not be empty'%(msg))

        # Apply TLS
        self.ApplyChangesPrefix (self._prefix, ['tls'], post)
        post.pop('tls')

        ModuleAuthBase._op_apply_changes (self, uri, post)
