import validations

from Table import *
from ModuleAuth import *

NOTE_PASSWD = _("Full path to the plain text password file.")

DATA_VALIDATION = [
    ('vserver!.*?!rule!.*?!auth!passwdfile', (validations.is_local_file_exists, 'cfg'))
]

HELPS = [
    ('modules_validators_plain', _("Plain text"))
]

class ModulePlain (ModuleAuthBase):
    PROPERTIES = ModuleAuthBase.PROPERTIES + [
        'passwdfile'
    ]

    METHODS = ['basic', 'digest']

    def __init__ (self, cfg, prefix, submit):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'plain', submit)

    def _op_render (self):
        txt  = ModuleAuthBase._op_render (self)

        table = TableProps()
        self.AddPropEntry (table, _("Password File"), "%s!passwdfile"%(self._prefix), NOTE_PASSWD)

        txt += "<h2>%s</h2>" % (_('Plain password file'))
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        pre = '%s!passwdfile' % (self._prefix)
        self.Validate_NotEmpty (post, pre, _('Password file can not be empty'))

        self.ApplyChanges ([], post, DATA_VALIDATION)
        ModuleAuthBase._op_apply_changes (self, uri, post)
