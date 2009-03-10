from Form import *
from Table import *
from ModuleHandler import *

HELPS = [
    ('modules_handlers_empty_gif', "1x1 Empty GIF")
]

class ModuleEmptyGif (ModuleHandler):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'empty_gif', cfg, prefix, submit_url)
        self.show_document_root = False

    def _op_render (self):
        return ''
    
    def _op_apply_changes (self, uri, post):
        None

