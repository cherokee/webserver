from Table import *
from ModuleAuth import *

# For gettext
N_ = lambda x: x

NOTE_HOST   = N_('MySQL server IP address.')
NOTE_PORT   = N_('Server port to connect to.')
NOTE_UNIX   = N_('Full path of Unix socket to communicate with the data base.')
NOTE_USER   = N_('User name for connecting to the database.')
NOTE_PASSWD = N_('Password for connecting to the database.')
NOTE_DB     = N_('Database name containing the user/password pair list.')
NOTE_SQL    = N_('SQL command to execute. ${user} is replaced with the user name.')
NOTE_HASH   = N_('Choose an encryption type for the password. Only suitable for the "Basic" authentication mechanism.')

HASHES = [
    ('',     'None'),
    ('md5',  'MD5'),
    ('sha1', 'SHA1')
]

HELPS = [
    ('modules_validators_mysql', "MySQL")
]

class ModuleMysql (ModuleAuthBase):
    PROPERTIES = ModuleAuthBase.PROPERTIES + [
        'host', 'port', 'unix_socket',
        'user', 'passwd', 'database',
        'query', 'hash'
    ]

    METHODS = ['basic', 'digest']

    def __init__ (self, cfg, prefix, submit):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'mysql', submit)

    def _op_render (self):
        txt  = ModuleAuthBase._op_render (self)

        is_basic = (self._cfg.get_val ("%s!methods"%(self._prefix)) in ["basic", None])

        table = TableProps()
        self.AddPropEntry (table, _("Host"), "%s!host"%(self._prefix), _(NOTE_HOST))
        self.AddPropEntry (table, _("Port"), "%s!port"%(self._prefix), _(NOTE_PORT))
        self.AddPropEntry (table, _("Unix Socket"), "%s!unix_socket"%(self._prefix), _(NOTE_UNIX))
        self.AddPropEntry (table, _("DB User"), "%s!user"%(self._prefix), _(NOTE_USER))
        self.AddPropEntry (table, _("DB Password"), "%s!passwd"%(self._prefix), _(NOTE_PASSWD))
        self.AddPropEntry (table, _("Database"), "%s!database"%(self._prefix), _(NOTE_DB))
        self.AddPropEntry (table, _("SQL Query"), "%s!query"%(self._prefix), _(NOTE_SQL))

        if not is_basic:
            self.AddPropOptions (table, _('Password Hash'), "%s!hash"%(self._prefix), HASHES, _(NOTE_HASH), disabled=1)
        else:
            self.AddPropOptions (table, _('Password Hash'), "%s!hash"%(self._prefix), HASHES, _(NOTE_HASH))

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

        # Check Hash
        hash_pre = "%s!hash"%(self._prefix)
        is_basic = (self._cfg.get_val ("%s!methods"%(self._prefix)) == "basic")

        if not is_basic:
            post.pop (hash_pre)
            del(self._cfg[hash_pre])

        ModuleAuthBase._op_apply_changes (self, uri, post)
