from Form import *
from Table import *
from Module import *

from consts import *

class ModuleRoundRobin (Module, FormHelper):
    def __init__ (self, cfg, prefix):
        Module.__init__ (self, 'round_robin', cfg, prefix)
        FormHelper.__init__ (self, 'round_robin', cfg)

    def _op_render (self):
        try:
            cfg = self._cfg[self._prefix]
            hosts = filter (lambda x: x != 'type', cfg)
        except:
            hosts = []

        # Render tables: as Hosts
        t1 = Table(2,1)
        t1 += ('Host', '')
        for host in hosts:
            pre = '%s!%s' % (self._prefix, host)
            e_host = Entry('%s!host'%(pre), 'text', self._cfg)
            t1 += (e_host, SUBMIT_DEL)

        en1 = Entry('new_host', 'text')
        t1 += (en1, SUBMIT_ADD)

        # Render tables: as Interpreters
        t2 = Table(3,1)
        t2 += ('Host', 'Interpreter', '')
        for host in hosts:
            pre = '%s!%s' % (self._prefix, host)
            e_host = Entry('%s!host'%(pre), 'text', self._cfg)
            e_inte = Entry('%s!interpreter'%(pre), 'text', self._cfg)
            t2 += (e_host, e_inte, SUBMIT_DEL)

        e_host = Entry('new_host', 'text')
        e_inte = Entry('new_interpreter', 'text')
        t2 += (e_host, e_inte, SUBMIT_ADD)

        # General selector
        props = {}
        props ['host']        = str(t1)
        props ['interpreter'] = str(t2)

        table = Table(2)
        e = self.AddTableOptions_w_Properties (table, "Information sources", 
                                               "%s!type"%(self._prefix), BALANCER_TYPES, props)
        return str(table) + e

    def _op_apply_changes (self, uri, post):
        self.ApplyChanges ([], post)

