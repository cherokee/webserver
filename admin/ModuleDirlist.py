import os
import validations 

from Form import *
from Table import *
from ModuleHandler import *
from configured import *

DATA_VALIDATION = [
    ('^vserver!.+?!.+?!.+?!handler!icon_dir',     validations.is_path),
    ('^vserver!.+?!.+?!.+?!handler!notice_files', validations.is_path_list),
]

DEFAULT_THEME = "default"

NOTE_THEME        = "Choose the listing theme."
NOTE_ICON_DIR     = "Web directory where the icon files are located. Default: <i>/icons</i>."
NOTE_NOTICE_FILES = "List of notice files to be inserted."

HELPS = [
    ('modules_handlers_dirlist', "Only listing")
]

class ModuleDirlist (ModuleHandler):
    PROPERTIES = [
        'size', 'date',
        'user', 'group',
        'theme', 'icon_dir',
        'notice_files', 'symlinks'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'dirlist', cfg, prefix, submit_url)

    def _op_render (self):
        txt = '<h2>Listing</h2>'
        table = TableProps()
        self.AddPropCheck (table, "Show Size",  "%s!size" %(self._prefix), True,  '')
        self.AddPropCheck (table, "Show Date",  "%s!date" %(self._prefix), True,  '')
        self.AddPropCheck (table, "Show User",  "%s!user" %(self._prefix), False, '')
        self.AddPropCheck (table, "Show Group", "%s!group"%(self._prefix), False, '')
        self.AddPropCheck (table, "Allow symbolic links", "%s!symlinks"%(self._prefix), True, '')
        txt += self.Indent(table)

        txt += '<h2>Theming</h2>'
        table  = TableProps()
        themes = self._get_theme_list()
        self.AddPropOptions_Reload (table, 'Theme', "%s!theme" % (self._prefix), themes, NOTE_THEME)
        self.AddPropEntry   (table, 'Icons dir',    "%s!icon_dir" % (self._prefix), NOTE_ICON_DIR)
        self.AddPropEntry   (table, 'Notice files', "%s!notice_files" % (self._prefix), NOTE_NOTICE_FILES)
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        checkboxes = ['size', 'date', 'user', 'group', 'symlinks']
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
