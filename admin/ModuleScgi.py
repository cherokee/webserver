from Form import *
from Table import *
from Module import *
from validations import *
from consts import *

from ModuleCgi import *

class ModuleScgi (ModuleCgiBase):
    PROPERTIES = ModuleCgiBase.PROPERTIES + [
        'balancer'
    ]

    def __init__ (self, cfg, prefix):
        ModuleCgiBase.__init__ (self, cfg, prefix, 'scgi')

    def _op_render (self):
        txt = '<h3>General</h3>'
        txt += ModuleCgiBase._op_render (self)

        txt += '<h3>FastCGI specific</h3>'
        table = Table(2)
        prefix = "%s!balancer" % (self._prefix)
        e = self.AddTableOptions_w_ModuleProperties (table, "Balancer", prefix, BALANCERS)

        txt += str(table) + e
        return txt

    def _op_apply_changes (self, uri, post):
        # Apply balancer changes
        pre  = "%s!balancer" % (self._prefix)
        name = self._cfg[pre].value

        props = module_obj_factory (name, self._cfg, pre)
        props._op_apply_changes (uri, post)
        
        # And CGI changes
        return ModuleCgiBase._op_apply_changes (self, uri, post)
