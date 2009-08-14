from Form import *
from Table import *
from ModuleHandler import *

# For gettext
N_ = lambda x: x

NOTE_INFORMATION = N_('Which information should be shown.')

options = [
    ('normal',             N_("Server Information")),
    ('just_about',         N_("Only version information")),
    ('connection_details', N_("Server Information + Connections"))
]

HELPS = [
    ('modules_handlers_server_info', N_("Server Information"))
]

class ModuleServerInfo (ModuleHandler):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'server_info', cfg, prefix, submit_url)
        self.show_document_root = False

    def _op_render (self):
        txt  = "<h2>%s</h2>" % (_('Privacy settings'))

        table = TableProps()
        self.AddPropOptions_Reload_Plain (table, _("Show Information"),
                                          "%s!type" % (self._prefix),
                                          options, _(NOTE_INFORMATION))
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        None
