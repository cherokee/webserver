from ModuleFile import *
from ModuleDirlist import *

class ModuleCommon:
    PROPERTIES = ModuleFile.PROPERTIES + ModuleDirlist.PROPERTIES

    def __init__ (self, cfg, prefix, submit_url):
        self._file = ModuleFile (cfg, prefix, submit_url)
        self._dirlist = ModuleDirlist (cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''
        txt += self._file._op_render()
        txt += self._dirlist._op_render()
        return txt

    def _op_apply_changes (self, uri, post):
        self._file._op_apply_changes (uri, post)
        self._dirlist._op_apply_changes (uri, post)
