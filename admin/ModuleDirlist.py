import os
import validations 

from Form import *
from Table import *
from Module import *
from configured import *

DATA_VALIDATION = [
    ('^vserver!.+?!.+?!.+?!handler!icon_dir',     validations.is_path),
    ('^vserver!.+?!.+?!.+?!handler!notice_files', validations.is_path_list),
]

DEFAULT_THEME = "default"

class ModuleDirlist (Module, FormHelper):
    PROPERTIES = [
        'size', 'date',
        'user', 'group',
        'theme', 'icon_dir',
        'notice_files'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'dirlist', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'dirlist', cfg)

    def _op_render (self):
        txt = '<h3>Listing</h3>'
        table = Table(2)
        self.AddTableCheckbox (table, "Show Size",  "%s!size" %(self._prefix), True)
        self.AddTableCheckbox (table, "Show Date",  "%s!date" %(self._prefix), True)
        self.AddTableCheckbox (table, "Show User",  "%s!user" %(self._prefix), False)
        self.AddTableCheckbox (table, "Show Group", "%s!group"%(self._prefix), False)
        txt += str(table)

        txt += '<h3>Theming</h3>'
        table  = Table(2)
        themes = self._get_theme_list()
        self.AddTableOptions (table, 'Theme',        "%s!theme" % (self._prefix), themes)
        self.AddTableEntry   (table, 'Icons dir',    "%s!icon_dir" % (self._prefix))
        self.AddTableEntry   (table, 'Notice files', "%s!notice_files" % (self._prefix))
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        checkboxes = ['size', 'date', 'user', 'group']
        self.ApplyChangesPrefix (self._prefix, checkboxes, post, DATA_VALIDATION)

    def _get_theme_list (self):
        themes = []
        for f in os.listdir(CHEROKEE_THEMEDIR):
            full = os.path.join(CHEROKEE_THEMEDIR, f)
            if os.path.isdir (full):
                themes.append((f,f))

        if not themes:
            themes = [(DEFAULT_THEME, DEFAULT_THEME)]

        return themes
