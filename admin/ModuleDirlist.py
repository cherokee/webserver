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
        self.ValidateChanges (post, DATA_VALIDATION)

        # Text entries
        for confkey in post:
            self._cfg[confkey] = post[confkey][0]

        # Rest: Checkboxes
        for k in ['size', 'date', 'user', 'group']:
            key = '%s!%s' % (self._prefix, k)
            if key in post:
                self._cfg[key] = post[key][0]
            else:
                self._cfg[key] = "0"
                
        
