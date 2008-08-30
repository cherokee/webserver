from Form import *
from Module import *

class ModuleHandler (Module, FormHelper):
    def __init__ (self, name, cfg, prefix, submit_url):
        FormHelper.__init__ (self, name, cfg)
        Module.__init__ (self, name, cfg, prefix, submit_url)

        self.show_document_root = True
