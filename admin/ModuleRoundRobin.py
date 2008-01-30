from Form import *
from Table import *
from Module import *

from consts import *

class ModuleRoundRobin (Module, FormHelper):
    def __init__ (self, cfg, prefix):
        Module.__init__ (self, 'round_robin', cfg, prefix)
        FormHelper.__init__ (self, 'round_robin', cfg)

    def _op_render (self):
        cfg = self._cfg[self._prefix]

        tipe  = cfg['type'].value
        hosts = filter (lambda x: x != 'type', cfg)

        # Render tables
        t1 = Table(2)
        for host in hosts:
            pre = '%s!%s' % (self._prefix, host)
            self.AddTableEntry (t1, 'Host', '%s!host'%(pre))

        t2 = Table(2)
        for host in hosts:
            pre = '%s!%s' % (self._prefix, host)
            self.AddTableEntry (t2, 'Host', '%s!host'%(pre))
            self.AddTableEntry (t2, 'Interpreter', '%s!interpreter'%(pre))

        props = {}
        props ['host']        = str(t1)
        props ['interpreter'] = str(t2)

        # General selector
        table = Table(2)
        e = self.AddTableOptions_w_Properties (table, "Information sources", 
                                               "%s!type"%(self._prefix), BALANCER_TYPES, props)
        return str(table) + e

    def _op_apply_changes (self, uri, post):
        self.ApplyChanges ([], post)

