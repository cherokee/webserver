from Form import *
from Table import *
from Module import *
import validations

NOTE_REQUEST = "Regular expression against which the request will be executed."

class ModuleRequest (Module, FormHelper):
    validation = [('tmp!new_rule!value', validations.is_regex)]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'request', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'request', cfg)

    def _op_render (self):
        table = TableProps()
        if self._prefix.startswith('tmp!'):
            self.AddPropEntry (table, 'Regular Expression', '%s!value'%(self._prefix), NOTE_REQUEST)
        else:
            self.AddPropEntry (table, 'Regular Expression', '%s!request'%(self._prefix), NOTE_REQUEST)
        return str(table)
        
    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print "ERROR, a 'value' entry is needed!"

        exts = values['value']
        self._cfg['%s!match!request'%(self._prefix)] = exts

    def get_name (self):
        return self._cfg.get_val ('%s!match!request'%(self._prefix))

    def get_type_name (self):
        return 'Regular Expression'
