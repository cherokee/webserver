from Table import *
from ModuleAuth import *

class ModuleMysql (ModuleAuthBase):
    PROPERTIES = ModuleAuthBase.PROPERTIES + [
        'host', 'port', 'unix_socket',
        'user', 'passwd', 'database',
        'query', 'use_md5_passwd'
    ]

    METHODS = ['basic', 'digest']

    def __init__ (self, cfg, prefix, submit):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'mysql', submit)

    def _op_render (self):
        table = Table(2)
        self.AddTableEntry (table, "Host", "%s!host"%(self._prefix))
        self.AddTableEntry (table, "Port", "%s!port"%(self._prefix))
        self.AddTableEntry (table, "Unix Socket", "%s!unix_socket"%(self._prefix))
        self.AddTableEntry (table, "DB User", "%s!user"%(self._prefix))
        self.AddTableEntry (table, "DB Password", "%s!passwd"%(self._prefix))
        self.AddTableEntry (table, "Database", "%s!database"%(self._prefix))
        self.AddTableEntry (table, "SQL Query", "%s!query"%(self._prefix))
        self.AddTableCheckbox (table, 'Use MD5 Passwords', "%s!use_md5_passwd"%(self._prefix), False)

        txt  = ModuleAuthBase._op_render (self)
        txt += '<h3>MySQL connection</h3>'
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        # These values must be filled out
        for key, msg in [('host', 'Host'),
                         ('user', 'DB User'),
                         ('query', 'SQL query'),
                         ('database', 'Database')]:
            pre = '%s!%s' % (self._prefix, key)
            self.Validate_NotEmpty (post, pre, '%s can not be empty'%(msg))
            
        # Apply TLS
        self.ApplyChangesPrefix (self._prefix, ['use_md5_passwd'], post)
        post.pop('use_md5_passwd')

        ModuleAuthBase._op_apply_changes (self, uri, post)
