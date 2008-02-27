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

        if len(self.METHODS) > 1:
            methods = VALIDATOR_METHODS
        else:
            method = self.METHODS[0]
            methods = filter (lambda x,m=method: x[0] == method, VALIDATOR_METHODS)

        self.AddTableOptions (table, "Methods", "%s!methods"%(self._prefix), methods)
        self.AddTableEntry   (table, "Realm",   "%s!realm"  %(self._prefix))
        self.AddTableEntry   (table, "Users",   "%s!users"  %(self._prefix))
        return str(table)

    def _op_apply_changes (self, uri, post):
        # Check Realm
        pre = '%s!realm' % (self._prefix)
        self.Validate_NotEmpty (post, pre, 'Realm can not be empty')

        # Auto-methods
        if len(self.METHODS) <= 1:
            self._cfg['%s!methods'%(self._prefix)] = self.METHODS[0]

        # the rest
        self.ApplyChanges ([], post, DATA_VALIDATION)
