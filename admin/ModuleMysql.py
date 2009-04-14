from Table import *
from ModuleAuth import *

NOTE_HOST   = _('MySQL server IP address.')
NOTE_PORT   = _('Server port to connect to.')
NOTE_UNIX   = _('Full path of Unix socket to communicate with the data base.')
NOTE_USER   = _('User name for connecting to the database.')
NOTE_PASSWD = _('Password for connecting to the database.')
NOTE_DB     = _('Database name containing the user/password pair list.')
NOTE_SQL    = _('SQL command to execute. ${user} is replaced with the user name.')
NOTE_MD5    = _('Active to use MD5 passwords.')


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
        self.AddPropEntry (table, _("Host"), "%s!host"%(self._prefix), NOTE_HOST)
        self.AddPropEntry (table, _("Port"), "%s!port"%(self._prefix), NOTE_PORT)
        self.AddPropEntry (table, _("Unix Socket"), "%s!unix_socket"%(self._prefix), NOTE_UNIX)
        self.AddPropEntry (table, _("DB User"), "%s!user"%(self._prefix), NOTE_USER)
        self.AddPropEntry (table, _("DB Password"), "%s!passwd"%(self._prefix), NOTE_PASSWD)
        self.AddPropEntry (table, _("Database"), "%s!database"%(self._prefix), NOTE_DB)
        self.AddPropEntry (table, _("SQL Query"), "%s!query"%(self._prefix), NOTE_SQL)
        self.AddPropCheck (table, _('Use MD5 Passwords'), "%s!use_md5_passwd"%(self._prefix), False, NOTE_MD5)

        txt += '<h2>%s</h2>' % (_('MySQL connection'))
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        # These values must be filled out
        for key, msg in [('user', _('DB User')),
                         ('query', _('SQL query')),
                         ('database', _('Database'))]:
            pre = '%s!%s' % (self._prefix, key)
            self.Validate_NotEmpty (post, pre, msg + _(' can not be empty'))

        # Apply TLS
        self.ApplyChangesPrefix (self._prefix, ['use_md5_passwd'], post)
        post.pop('use_md5_passwd')

        ModuleAuthBase._op_apply_changes (self, uri, post)
