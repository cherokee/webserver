from Form import *
from Table import *
from Module import *
import validations

NOTE_DIRECTORY = "Public Web Directory to which content the configuration will be applied."

class ModuleDirectory (Module, FormHelper):
    validation = [('tmp!new_rule!value', validations.is_dir_formated)]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'directory', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'directory', cfg)

    def _op_render (self):
        table = TableProps()
        if self._prefix.startswith('tmp!'):
            self.AddPropEntry (table, 'Web Directory', '%s!value'%(self._prefix), NOTE_DIRECTORY)
        else:
            self.AddPropEntry (table, 'Web Directory', '%s!directory'%(self._prefix), NOTE_DIRECTORY)
        return str(table)
        
    def apply_cfg (self, values):
        if values.has_key('value'):
            dir_name = values['value']
            self._cfg['%s!match!directory'%(self._prefix)] = dir_name

    def get_name (self):
        return self._cfg.get_val ('%s!match!directory'%(self._prefix))

    def get_type_name (self):
        return self._id.capitalize()
