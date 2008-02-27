import validations

from Table import *
from ModuleAuth import *

DATA_VALIDATION = [
    ('vserver!.*?!(directory|extensions|request)!.*?!passwdfile', validations.is_local_file_exists)
]

class ModuleHtdigest (ModuleAuthBase):
    PROPERTIES = ModuleAuthBase.PROPERTIES + [
        'passwdfile'
    ]

    METHODS = ['basic', 'digest']

    def __init__ (self, cfg, prefix, submit):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'htdigest', submit)

    def _op_render (self):
        table = Table(2)
        self.AddTableEntry (table, "Password File", "%s!passwdfile"%(self._prefix))

        txt  = ModuleAuthBase._op_render (self)
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        pre = '%s!passwdfile' % (self._prefix)
        self.Validate_NotEmpty (post, pre, 'Password file can not be empty')

        self.ApplyChanges ([], post, DATA_VALIDATION)
        ModuleAuthBase._op_apply_changes (self, uri, post)
