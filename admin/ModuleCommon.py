from ModuleFile import *
from ModuleDirlist import *

class ModuleCommon:
    PROPERTIES = ModuleFile.PROPERTIES + ModuleDirlist.PROPERTIES

    def __init__ (self, cfg, prefix, submit_url):
        self.errors = {}

        self._file    = ModuleFile    (cfg, prefix, submit_url)
        self._dirlist = ModuleDirlist (cfg, prefix, submit_url)

    def _op_render (self):
        # Copy errors to the modules, 
        # they may need to print them
        self._copy_errors (self, self._file)
        self._copy_errors (self, self._dirlist)

        txt = ''
        txt += self._file._op_render()
        txt += self._dirlist._op_render()
        return txt

    def _op_apply_changes (self, uri, post):
        # Copy errors from the child modules
        self._copy_errors (self._file,    self)
        self._copy_errors (self._dirlist, self)

        # Apply the changes
        self._file._op_apply_changes (uri, post)
        self._dirlist._op_apply_changes (uri, post)

    def _copy_errors (self, _from, _to):
        for e in _from.errors:
            _to.errors[e] = _from.errors[e]
