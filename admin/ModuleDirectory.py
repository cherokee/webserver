from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_DIRECTORY = N_("Public Web Directory to which content the configuration will be applied.")

class ModuleDirectory (Module, FormHelper):
    validation = [('tmp!new_rule!value', validations.is_dir_formated)]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'directory', cfg)
        Module.__init__ (self, 'directory', cfg, prefix, submit_url)

    def _op_render (self):
        table = TableProps()
        if self._prefix.startswith('tmp!'):
            self.AddPropEntry (table, _('Web Directory'), '%s!value'%(self._prefix), _(NOTE_DIRECTORY))
        else:
            self.AddPropEntry (table, _('Web Directory'), '%s!directory'%(self._prefix), _(NOTE_DIRECTORY))
        return str(table)

    def apply_cfg (self, values):
        if values.has_key('value'):
            dir_name = values['value']
            self._cfg['%s!directory'%(self._prefix)] = dir_name

    def get_name (self):
        return self._cfg.get_val ('%s!directory'%(self._prefix))

    def get_type_name (self):
        return self._id.capitalize()
