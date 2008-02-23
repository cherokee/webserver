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

    def __init__ (self, cfg, prefix, submit):
        ModuleCgiBase.__init__ (self, cfg, prefix, 'scgi', submit)

    def _op_render (self):
        txt = '<h3>General</h3>'
        txt += ModuleCgiBase._op_render (self)

        txt += '<h3>SCGI specific</h3>'

        table = Table(2)
        prefix = "%s!balancer" % (self._prefix)
        e = self.AddTableOptions_Reload (table, "Balancer", prefix, BALANCERS)
        txt += str(table) + self.Indent(e)
        return txt

    def _op_apply_changes (self, uri, post):
        # Apply balancer changes
        pre  = "%s!balancer" % (self._prefix)

        new_balancer = post.pop(pre)
        if new_balancer:
            self._cfg[pre] = new_balancer

        cfg  = self._cfg[pre]
        if cfg and cfg.value:
            name = cfg.value
            props = module_obj_factory (name, self._cfg, pre, self.submit_url)
            props._op_apply_changes (uri, post)
        
        # And CGI changes
        return ModuleCgiBase._op_apply_changes (self, uri, post)
