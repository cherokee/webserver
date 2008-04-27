from Form import *
from Table import *
from Module import *

class ModuleDefault (Module, FormHelper):
    validation = []

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'default', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'default', cfg)

    def _op_render (self):
        return ""
        
    def apply_cfg (self, values):
        None

    def get_name (self):
        return self._id.capitalize()

    def get_type_name (self):
        return self._id.capitalize()
