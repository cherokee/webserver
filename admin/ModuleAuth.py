import validations 

from Form import *
from Table import *
from Module import *
from consts import *

DATA_VALIDATION = [
    ('vserver!.*?!(directory|extensions|request)!.*?!auth!users', validations.is_safe_id_list)
]

class ModuleAuthBase (Module, FormHelper):
    PROPERTIES = [
        'methods',
        'realm',
        'users'
    ]

    def __init__ (self, cfg, prefix, name, submit):
        Module.__init__ (self, name, cfg, prefix, submit)
        FormHelper.__init__ (self, name, cfg)

    def _op_render (self):
        table = Table(2)
        self.AddTableOptions (table, "Methods", "%s!methods"%(self._prefix), VALIDATOR_METHODS)
        self.AddTableEntry   (table, "Realm",   "%s!realm"  %(self._prefix))
        self.AddTableEntry   (table, "Users",   "%s!users"  %(self._prefix))
        return str(table)

    def _op_apply_changes (self, uri, post):
        pre = '%s!realm' % (self._prefix)
        self.Validate_NotEmpty (post, pre, 'Realm can not be empty')

        self.ApplyChanges ([], post, DATA_VALIDATION)
