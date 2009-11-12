from Form import *
from Table import *
from Module import *

import validations

NOTE_DB_DIR  = N_("Directory where the RRDtool databases should be written.")
NOTE_RRDTOOL = N_("Path to the rrdtool binary. By default the server will look in the PATH.")

class ModuleRrd (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'rrd', cfg)
        Module.__init__ (self, 'rrd', cfg, prefix, submit_url)

        self.validation = [
            ('%s!database_dir'%(prefix), (validations.is_local_dir_exists, 'cfg')),
            ('%s!rrdtool_path'%(prefix),  validations.is_exec_path),
        ]

    def _op_render (self):
        txt = ''

        table = TableProps()
        self.AddPropEntry (table, _('RRD Database directory'), '%s!database_dir'%(self._prefix), _(NOTE_DB_DIR), optional=True)
        self.AddPropEntry (table, _('Custom rrdtool binary'),  '%s!rrdtool_path'%(self._prefix), _(NOTE_RRDTOOL), optional=True)
        txt += str(table)
        return txt
