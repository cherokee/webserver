from ModuleAuth import *

class ModulePam (ModuleAuthBase):
    PROPERTIES = ModuleAuthBase.PROPERTIES

    METHODS = ['basic']

    def __init__ (self, cfg, prefix, submit):
        ModuleAuthBase.__init__ (self, cfg, prefix, 'pam', submit)

    def _op_render (self):
        return ModuleAuthBase._op_render (self)

    def _op_apply_changes (self, uri, post):
        return ModuleAuthBase._op_apply_changes (self, uri, post)
