from Table import *
from ModuleAuth import *

NOTE_HOST   = 'MySQL server IP address.'
NOTE_PORT   = 'Server port to connect to.'
NOTE_UNIX   = 'Full path of Unix socket to communicate with the data base.'
NOTE_USER   = 'User name for connecting to the database.'
NOTE_PASSWD = 'Password for connecting to the database.'
NOTE_DB     = 'Database name containing the user/password pair list.'
NOTE_SQL    = 'SQL command to execute. ${user} is replaced with the user name.'
NOTE_MD5    = 'Active to use MD5 passwords.'


HELPS = [
    ('modules_validators_mysql', "MySQL")
]

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
        txt  = ModuleAuthBase._op_render (self)

        table = TableProps()
        self.AddPropEntry (table, "Host", "%s!host"%(self._prefix), NOTE_HOST)
        self.AddPropEntry (table, "Port", "%s!port"%(self._prefix), NOTE_PORT)
        self.AddPropEntry (table, "Unix Socket", "%s!unix_socket"%(self._prefix), NOTE_UNIX)
        self.AddPropEntry (table, "DB User", "%s!user"%(self._prefix), NOTE_USER)
        self.AddPropEntry (table, "DB Password", "%s!passwd"%(self._prefix), NOTE_PASSWD)
        self.AddPropEntry (table, "Database", "%s!database"%(self._prefix), NOTE_DB)
        self.AddPropEntry (table, "SQL Query", "%s!query"%(self._prefix), NOTE_SQL)
        self.AddPropCheck (table, 'Use MD5 Passwords', "%s!use_md5_passwd"%(self._prefix), False, NOTE_MD5)

        txt += '<h2>MySQL connection</h2>'
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        # These values must be filled out
        for key, msg in [('user', 'DB User'),
                         ('query', 'SQL query'),
                         ('database', 'Database')]:
            pre = '%s!%s' % (self._prefix, key)
            self.Validate_NotEmpty (post, pre, '%s can not be empty'%(msg))

        # Apply TLS
        self.ApplyChangesPrefix (self._prefix, ['use_md5_passwd'], post)
        post.pop('use_md5_passwd')

        ModuleAuthBase._op_apply_changes (self, uri, post)
