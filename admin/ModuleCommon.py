from Form import *
from Table import *

from ModuleHandler import *
from ModuleFile import *
from ModuleDirlist import *

# For gettext
N_ = lambda x: x

NOTE_PATHINFO = N_("Allow extra tailing paths")
NOTE_DIRLIST  = N_("Allow to list directory contents")

HELPS = [
    ('modules_handlers_common', N_("List & Send"))
]

class ModuleCommon (ModuleHandler):
    PROPERTIES = ModuleFile.PROPERTIES + ModuleDirlist.PROPERTIES + [
        'allow_pathinfo', 'allow_dirlist'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'common', cfg, prefix, submit_url)

        self._file    = ModuleFile    (cfg, prefix, submit_url)
        self._dirlist = ModuleDirlist (cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''

        # Local properties
        table = TableProps()
        self.AddPropCheck (table, _('Allow PathInfo'), '%s!allow_pathinfo'%(self._prefix), False, _(NOTE_PATHINFO))
        self.AddPropCheck (table, _('Allow Directory Listing'), '%s!allow_dirlist'%(self._prefix), True, _(NOTE_DIRLIST))

        txt = '<h2>%s</h2>' % (_('Parsing'))
        txt += self.Indent(table)

        # Copy errors to the modules,
        # they may need to print them
        self._copy_errors (self, self._file)
        self._copy_errors (self, self._dirlist)

        txt += self._file._op_render()
        txt += self._dirlist._op_render()

        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyCheckbox (post, '%s!allow_pathinfo'%(self._prefix))
        self.ApplyCheckbox (post, '%s!allow_dirlist'%(self._prefix))

        # Apply the changes
        self._file._op_apply_changes (uri, post)
        self._dirlist._op_apply_changes (uri, post)

        # Copy errors from the child modules
        self._copy_errors (self._file,    self)
        self._copy_errors (self._dirlist, self)

    def _copy_errors (self, _from, _to):
        for e in _from.errors:
            _to.errors[e] = _from.errors[e]
