from Form import *
from Table import *
from Module import *
from validations import *
from consts import *
from ModuleBalancer import *

class ModuleMirror (Module, FormHelper):
    PROPERTIES = [
        'balancer'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'mirror', cfg)
        Module.__init__ (self, 'mirror', cfg, prefix, submit_url)

    def _op_render (self):
        prefix = "%s!balancer" % (self._prefix)

        table = TableProps()
        e = self.AddPropOptions_Reload (table, "Balancer", prefix,
                                        modules_available(BALANCERS), NOTE_BALANCER,
                                        default_type='host',
                                        allow_type_change=False)

        txt  = "<h2>Load balancing options</h2>"
        txt += self.Indent (str(table) + e)
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
        
        # And apply the rest
        self.ApplyChangesPrefix (self._prefix, [], post)


