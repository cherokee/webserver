from Form import *
from Table import *
from Module import *
from validations import *

DATA_VALIDATION = [
    ('^vserver!.+?!.+?!.+?!handler!icon_dir',     validate_path),
    ('^vserver!.+?!.+?!.+?!handler!notice_files', validate_path_list),
]

class ModuleDirlist (Module, FormHelper):
    def __init__ (self, cfg, prefix):
        Module.__init__ (self, 'dirlist', cfg, prefix)
        FormHelper.__init__ (self, 'dirlist', cfg)

    def _op_render (self):
        txt = '<h3>Listing</h3>'
        table = Table(2)
        self.AddTableCheckbox (table, "Show size", "%s!size" % (self._prefix))
        self.AddTableCheckbox (table, "Show date", "%s!date" % (self._prefix))
        self.AddTableCheckbox (table, "Show user", "%s!user" % (self._prefix))
        self.AddTableCheckbox (table, "Show group", "%s!group" % (self._prefix))
        txt += str(table)
        
        txt += '<h3>Theming</h3>'
        table = Table(2)
        self.AddTableEntry    (table, 'Theme name',   "%s!theme" % (self._prefix))
        self.AddTableEntry    (table, 'Icons dir',    "%s!icon_dir" % (self._prefix))
        self.AddTableEntry    (table, 'Notice files', "%s!notice_files" % (self._prefix))
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        checkboxes = ['size', 'date', 'user', 'group']
        self.ApplyChangesPrefix (self._prefix, checkboxes, post, DATA_VALIDATION)
