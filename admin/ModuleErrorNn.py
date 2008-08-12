import validations

from Form import *
from Table import *
from Module import *
from consts import *

class ModuleErrorNn (Module, FormHelper):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'error_nn', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'error_nn', cfg)
        
    def _op_render (self):
        return ''

    def _op_apply_changes (self, uri, post):
        None
