from Form import *
from Table import *
from Module import *
import validations

NOTE_EXISTS = "File list to which -if present- content the configuration will be applied."

class ModuleExists (Module, FormHelper):
    validation = [('tmp!new_rule!value', validations.is_safe_id_list)]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'exists', cfg)
        Module.__init__ (self, 'exists', cfg, prefix, submit_url)

    def _op_render (self):
        table = TableProps()
        if self._prefix.startswith('tmp!'):
            self.AddPropEntry (table, 'Files', '%s!value'%(self._prefix), NOTE_EXISTS)
        else:
            self.AddPropEntry (table, 'Files', '%s!exists'%(self._prefix), NOTE_EXISTS)
        return str(table)
        
    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print "ERROR, a 'value' entry is needed!"

        exts = values['value']
        self._cfg['%s!match!exists'%(self._prefix)] = exts

    def get_name (self):
        return self._cfg.get_val ('%s!match!exists'%(self._prefix))

    def get_type_name (self):
        return "File Exists"
