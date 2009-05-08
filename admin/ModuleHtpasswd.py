import validations

from Table import *
from ModuleAuth import *

# For gettext
N_ = lambda x: x

NOTE_PASSWD = N_("Full path to the Htpasswd formated password file.")

DATA_VALIDATION = [
    ('vserver!.*?!rule!.*?!auth!passwdfile', (validations.is_local_file_exists, 'cfg'))
]

HELPS = [
    ('modules_validators_htpasswd', "Htpasswd")
]

class ModuleHtpasswd (ModuleAuthBase):
    PROPERTIES = ModuleAuthBase.PROPERTIES + [
        'passwdfile'
    ]

    METHODS = ['basic']

    def __init__ (self, cfg, prefix, submit):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'htpasswd', submit)

    def _op_render (self):
        txt  = ModuleAuthBase._op_render (self)

        table = TableProps()
        self.AddPropEntry (table, _("Password File"), "%s!passwdfile"%(self._prefix), _(NOTE_PASSWD))

        txt += "<h2>%s</h2>" % (_('Htpasswd file'))
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        pre = '%s!passwdfile' % (self._prefix)
        self.Validate_NotEmpty (post, pre, _('Password file can not be empty'))

        self.ApplyChanges ([], post, DATA_VALIDATION)
        ModuleAuthBase._op_apply_changes (self, uri, post)
