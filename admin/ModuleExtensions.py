from Form import *
from Table import *
from Module import *
import validations

NOTE_EXTENSIONS = "File extension list to which content the configuration will be applied."

class ModuleExtensions (Module, FormHelper):
    validation = [('tmp!new_rule!value', validations.is_safe_id_list)]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'extensions', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'extensions', cfg)

    def _op_render (self):
        table = TableProps()
        if self._prefix.startswith('tmp!'):
            self.AddPropEntry (table, 'Extensions', '%s!value'%(self._prefix), NOTE_EXTENSIONS)
        else:
            self.AddPropEntry (table, 'Extensions', '%s!extensions'%(self._prefix), NOTE_EXTENSIONS)
        return str(table)
        
    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print "ERROR, a 'value' entry is needed!"

        exts = values['value']
        self._cfg['%s!match!extensions'%(self._prefix)] = exts

    def get_name (self):
        return self._cfg.get_val ('%s!match!extensions'%(self._prefix))

    def get_type_name (self):
        return self._id.capitalize()
