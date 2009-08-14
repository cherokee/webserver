from Form import *
from Table import *
from ModuleHandler import *
from consts import *

# For gettext
N_ = lambda x: x

NOTE_ERRORS = N_('HTTP Error that you be used to reply the request.')

HELPS = [
    ('modules_handlers_custom_error', N_("HTTP Custom Error"))
]

class ModuleCustomError (ModuleHandler):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'custom_error', cfg, prefix, submit_url)
        self.show_document_root = False

    def _op_render (self):
        table = TableProps()
        self.AddPropOptions_Reload_Plain (table, _("HTTP Error"), "%s!error" % (self._prefix), ERROR_CODES, _(NOTE_ERRORS))
        return str(table)

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, [], post)

