import validations 

from Form import *
from Table import *
from Module import *
from consts import *

DATA_VALIDATION = [
    ('vserver!.*?!(directory|extensions|request)!.*?!auth!users', validations.is_safe_id_list)
]

NOTE_METHODS = 'Allowed HTTP Authentication methods.'
NOTE_REALM   = 'Name associated with the protected resource.'
NOTE_USERS   = 'User filter. List of allowed users.'

class ModuleAuthBase (Module, FormHelper):
    PROPERTIES = [
        'methods',
        'realm',
        'users'
    ]

    def __init__ (self, cfg, prefix, name, submit):
        FormHelper.__init__ (self, name, cfg)
        Module.__init__ (self, name, cfg, prefix, submit)

    def _op_render (self):
        txt = ''

        if len(self.METHODS) > 1:
            methods = VALIDATOR_METHODS
        else:
            method = self.METHODS[0]
            methods = filter (lambda x,m=method: x[0] == method, VALIDATOR_METHODS)

        table = TableProps()
        self.AddPropOptions_Reload (table, "Methods", "%s!methods"%(self._prefix), methods, NOTE_METHODS)
        self.AddPropEntry   (table, "Realm",   "%s!realm"  %(self._prefix), NOTE_REALM)
        self.AddPropEntry   (table, "Users",   "%s!users"  %(self._prefix), NOTE_USERS)

        txt += "<h2>Authentication Details</h2>"
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        # Check Realm
        pre = '%s!realm' % (self._prefix)
        self.Validate_NotEmpty (post, pre, 'Realm can not be empty')

        # Auto-methods
        if len(self.METHODS) <= 1:
            self._cfg['%s!methods'%(self._prefix)] = self.METHODS[0]

        # the rest
        self.ApplyChanges ([], post, DATA_VALIDATION)
