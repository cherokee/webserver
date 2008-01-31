from Table import *
from ModuleAuth import *

class ModuleMysql (ModuleAuthBase):
    def __init__ (self, cfg, prefix):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'mysql')

    def _op_render (self):
        table = Table(2)
        self.AddTableEntry (table, "Host", "%s!host"%(self._prefix))
        self.AddTableEntry (table, "Port", "%s!port"%(self._prefix))
        self.AddTableEntry (table, "Unix Socket", "%s!unix_socket"%(self._prefix))
        self.AddTableEntry (table, "DB User", "%s!user"%(self._prefix))
        self.AddTableEntry (table, "DB Password", "%s!user"%(self._prefix))
        self.AddTableEntry (table, "Database", "%s!database"%(self._prefix))
        self.AddTableEntry (table, "Query", "%s!query"%(self._prefix))
        self.AddTableCheckbox (table, 'Use MD5 Passwords', "%s!use_md5_passwd"%(self._prefix), False)

        txt  = ModuleAuthBase._op_render (self)
        txt += '<h3>MySQL connection</h3>'
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['use_md5_passwd'], post)

        post2 = filter(lambda x: x!='use_md5_passwd', post)
        ModuleAuthBase._op_apply_changes (self, uri, post2)
