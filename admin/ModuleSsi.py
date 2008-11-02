from Form import *
from ModuleHandler import *

HELPS = [
    ('modules_handlers_ssi', "Server Side Includes")
]

class ModuleSsi (ModuleHandler):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'ssi', cfg, prefix, submit_url)
        self.show_document_root = False

    def _op_render (self):
        return ''

    def _op_apply_changes (self, uri, post):
        None
