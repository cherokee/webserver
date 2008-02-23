from Form import *
from Table import *
from Module import *
from validations import *
from consts import *

from ModuleCgi import *

FCGI_PARAMS_COMMENT = """
The FastCGI handler can distribute the workload between multiple
servers. Thus, a balancer must be configured in order to point the
handler to the application servers.
"""

class ModuleFcgi (ModuleCgiBase):
    PROPERTIES = ModuleCgiBase.PROPERTIES + [
        'balancer'
    ]

    def __init__ (self, cfg, prefix, submit):
        ModuleCgiBase.__init__ (self, cfg, prefix, 'fcgi', submit)

    def _op_render (self):
        txt = '<h3>General</h3>'
        txt += ModuleCgiBase._op_render (self)

        txt += '<h3>FastCGI specific</h3>'
        txt += self.Dialog (FCGI_PARAMS_COMMENT)

        table = Table(2)
        prefix = "%s!balancer" % (self._prefix)
        assert (self.submit_url)
        e = self.AddTableOptions_Reload (table, "Balancer", prefix, BALANCERS)
        txt += str(table) + self.Indent(e)
        return txt

    def _op_apply_changes (self, uri, post):
        # Apply balancer changes
        pre = "%s!balancer" % (self._prefix)

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
