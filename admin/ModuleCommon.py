from Form import *
from Table import *

from ModuleHandler import *
from ModuleFile import *
from ModuleDirlist import *

NOTE_PATHINFO = "Allow extra tailing paths"

HELPS = [
    ('modules_handlers_common', "List & Send")
]

class ModuleCommon (ModuleHandler):
    PROPERTIES = ModuleFile.PROPERTIES + ModuleDirlist.PROPERTIES + [
        'allow_pathinfo'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'common', cfg, prefix, submit_url)

        self._file    = ModuleFile    (cfg, prefix, submit_url)
        self._dirlist = ModuleDirlist (cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''

        # Local properties
        table = TableProps()
        self.AddPropCheck (table, 'Allow PathInfo', '%s!allow_pathinfo'%(self._prefix), False, NOTE_PATHINFO)

        txt = '<h2>Parsing</h2>'
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

        # Copy errors from the child modules
        self._copy_errors (self._file,    self)
        self._copy_errors (self._dirlist, self)

        # Apply the changes
        self._file._op_apply_changes (uri, post)
        self._dirlist._op_apply_changes (uri, post)

    def _copy_errors (self, _from, _to):
        for e in _from.errors:
            _to.errors[e] = _from.errors[e]
