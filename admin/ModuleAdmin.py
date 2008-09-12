from Form import *
from ModuleHandler import *

HELPS = [
    ('modules_handlers_admin', "Remote Administration")
]

class ModuleAdmin (ModuleHandler):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit):
        ModuleHandler.__init__ (self, 'admin', cfg, prefix, submit)
        self.show_document_root = False

    def _op_render (self):
        return ''

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, [], post)
