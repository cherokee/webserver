from Table import *
from ModuleAuth import *

class ModuleLdap (ModuleAuthBase):
    def __init__ (self, cfg, prefix):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'ldap')

    def _op_render (self):
        table = Table(2)
        self.AddTableEntry (table, "Server", "%s!server"%(self._prefix))
        self.AddTableEntry (table, "Port", "%s!port"%(self._prefix))
        self.AddTableEntry (table, "Bind Domain", "%s!bind_dn"%(self._prefix))
        self.AddTableEntry (table, "Bind Password", "%s!bind_pw"%(self._prefix))
        self.AddTableEntry (table, "Base Domain", "%s!base_dn"%(self._prefix))
        self.AddTableEntry (table, "Filter", "%s!filter"%(self._prefix))
        self.AddTableCheckbox (table, 'Use TLS', "%s!tls"%(self._prefix))
        self.AddTableEntry (table, "CA File", "%s!ca_file"%(self._prefix))

        txt  = ModuleAuthBase._op_render (self)
        txt += '<h3>LDAP connection</h3>'
        txt += str(table)
        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['tls'], post)

        post2 = filter(lambda x: x!='tls', post)
        ModuleAuthBase._op_apply_changes (self, uri, post2)
