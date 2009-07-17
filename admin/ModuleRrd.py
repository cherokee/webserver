from Form import *
from Table import *
from Module import *

import validations

NOTE_DB_DIR    = N_("Advanced: Directory where the RRDtool databases should be written.")
NOTE_RRDTOOL   = N_("Advanced: Path to the rrdtool binary. By default the server will look in the PATH.")
NOTE_RENDERING = N_("Advanded: How often usage graphics should be regenerated. (Default: 60 seconds)")

class ModuleRrd (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'rrd', cfg)
        Module.__init__ (self, 'rrd', cfg, prefix, submit_url)

        self.validation = [
            ('%s!database_dir'%(prefix), (validations.is_local_dir_exists, 'cfg')),
            ('%s!rrdtool_path'%(prefix),  validations.is_exec_path),
            ('%s!render_elapse'%(prefix), validations.is_number_gt_0)
        ]

    def _op_render (self):
        txt = ''

        table = TableProps()
        self.AddPropEntry (table, _('RRD Database directory'),           '%s!database_dir'%(self._prefix), _(NOTE_DB_DIR))
        self.AddPropEntry (table, _('Custom rrdtool binary'),            '%s!rrdtool_path'%(self._prefix), _(NOTE_RRDTOOL))
        self.AddPropEntry (table, _('Rendering interval (<i>secs</i>)'), '%s!render_elapse'%(self._prefix), _(NOTE_RENDERING))
        txt += str(table)
        return txt
