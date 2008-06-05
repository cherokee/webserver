from Form import *
from Table import *
from Module import *
import validations

NOTE_COUNTRIES = "Country list to which content the configuration will be applied."

class ModuleGeoip (Module, FormHelper):
    validation = [('tmp!new_rule!value', validations.is_safe_id_list)]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'geoip', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'geoip', cfg)

    def _op_render (self):
        table = TableProps()
        if self._prefix.startswith('tmp!'):
            self.AddPropEntry (table, 'Countries', '%s!value'%(self._prefix), NOTE_COUNTRIES)
        else:
            self.AddPropEntry (table, 'Countries', '%s!countries'%(self._prefix), NOTE_COUNTRIES)
        return str(table)
        
    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print "ERROR, a 'value' entry is needed!"

        exts = values['value']
        self._cfg['%s!match!countries'%(self._prefix)] = exts

    def get_name (self):
        return self._cfg.get_val ('%s!match!countries'%(self._prefix))

    def get_type_name (self):
        return self._id.capitalize()
