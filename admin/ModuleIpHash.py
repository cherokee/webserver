from Form import *
from Table import *
from Module import *
from ModuleBalancer import *

from consts import *

RR_COMMENT = """
The <i>IP Hash</i> balancer distributes connections between upstreams
based on the IP-address of the client.
"""

class ModuleIpHash (ModuleBalancerGeneric):
    def __init__ (self, cfg, prefix, submit_url):
        ModuleBalancerGeneric.__init__ (self, cfg, prefix, submit_url, 'ip_hash')

    def _op_render (self):
        txt = ModuleBalancerGeneric._op_render (self)
        return txt

    def _op_apply_changes (self, uri, post):
        txt = ModuleBalancerGeneric._op_apply_changes (self, uri, post)
        return txt
        
