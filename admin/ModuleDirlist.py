import os
import validations 

from Form import *
from Table import *
from ModuleHandler import *
from configured import *

DEFAULT_THEME = "default"

NOTE_THEME        = _("Choose the listing theme.")
NOTE_ICON_DIR     = _("Web directory where the icon files are located. Default: <i>/icons</i>.")
NOTE_NOTICE_FILES = _("List of notice files to be inserted.")
NOTE_HIDDEN_FILES = _("List of files that should not be listed.")

DATA_VALIDATION = [
    ('vserver!.+?!rule!.+?!handler!icon_dir',     validations.is_path),
    ('vserver!.+?!rule!.+?!handler!notice_files', validations.is_path_list),
    ('vserver!.+?!rule!.+?!handler!hidden_files', validations.is_list)
]

HELPS = [
    ('modules_handlers_dirlist', _("Only listing"))
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
        txt = '<h2>%s</h2>' % (_('Listing'))
        table = TableProps()
        self.AddPropCheck (table, _("Show Size"),  "%s!size" %(self._prefix), True,  '')
        self.AddPropCheck (table, _("Show Date"),  "%s!date" %(self._prefix), True,  '')
        self.AddPropCheck (table, _("Show User"),  "%s!user" %(self._prefix), False, '')
        self.AddPropCheck (table, _("Show Group"), "%s!group"%(self._prefix), False, '')
        self.AddPropCheck (table, _("Allow symbolic links"), "%s!symlinks"%(self._prefix), True, '')
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_('Theming'))
        table  = TableProps()
        themes = self._get_theme_list()
        self.AddPropOptions_Reload (table, _('Theme'), "%s!theme" % (self._prefix), themes, NOTE_THEME)
        self.AddPropEntry (table, _('Icons dir'),    "%s!icon_dir" % (self._prefix), NOTE_ICON_DIR)
        self.AddPropEntry (table, _('Notice files'), "%s!notice_files" % (self._prefix), NOTE_NOTICE_FILES)
        self.AddPropEntry (table, _('Hidden files'), "%s!hidden_files" % (self._prefix), NOTE_HIDDEN_FILES)
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
