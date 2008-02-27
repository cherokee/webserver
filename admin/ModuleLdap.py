from Table import *
from ModuleAuth import *

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
        table = Table(2)
        self.AddTableEntry (table, "Server", "%s!server"%(self._prefix))
        self.AddTableEntry (table, "Port", "%s!port"%(self._prefix))
        self.AddTableEntry (table, "Bind Domain", "%s!bind_dn"%(self._prefix))
        self.AddTableEntry (table, "Bind Password", "%s!bind_pw"%(self._prefix))
        self.AddTableEntry (table, "Base Domain", "%s!base_dn"%(self._prefix))
        self.AddTableEntry (table, "Filter", "%s!filter"%(self._prefix))
        self.AddTableCheckbox (table, 'Use TLS', "%s!tls"%(self._prefix), False)
        self.AddTableEntry (table, "CA File", "%s!ca_file"%(self._prefix))

        txt  = ModuleAuthBase._op_render (self)
        txt += '<h3>LDAP connection</h3>'
        txt += str(table)
        return txt

    def _op_apply_changes (self, uri, post):
        # These values must be filled out
        for key, msg in [('server', 'Server'),
                         ('bind_dn', 'Bind Domain'),
                         ('base_dn', 'Base Domain')]:
            pre = '%s!%s' % (self._prefix, key)
            self.Validate_NotEmpty (post, pre, '%s can not be empty'%(msg))
            
        # Apply TLS
        self.ApplyChangesPrefix (self._prefix, ['tls'], post)
        post.pop('tls')

        ModuleAuthBase._op_apply_changes (self, uri, post)
