from Form import *
from Table import *
from Module import *
from ModuleBalancer import *

from consts import *

RR_COMMENT = """
The <i>Round Robin</i> balancer can either access remote servers or
launch local application servers and connect to them.
"""

class ModuleRoundRobin (ModuleBalancerGeneric):
    def __init__ (self, cfg, prefix, submit_url):
        ModuleBalancerGeneric.__init__ (self, cfg, prefix, submit_url, 'round_robin')

    def _op_render (self):
        txt = ModuleBalancerGeneric._op_render (self)
        return txt

    def _op_apply_changes (self, uri, post):
        txt = ModuleBalancerGeneric._op_apply_changes (self, uri, post)
        return txt
        
