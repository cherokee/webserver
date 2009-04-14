from Form import *
from Table import *
from ModuleHandler import *

NOTE_INFORMATION = _('Which information should be shown.')

options = [
    ('normal',             _("Server Information")),
    ('just_about',         _("Only version information")),
    ('connection_details', _("Server Information + Connections"))
]

HELPS = [
    ('modules_handlers_server_info', _("Server Information"))
]

class ModuleServerInfo (ModuleHandler):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'server_info', cfg, prefix, submit_url)
        self.show_document_root = False

    def _op_render (self):
        txt  = "<h2>%s</h2>" % (_('Privacy settings'))

        table = TableProps()
        self.AddPropOptions_Reload (table, _("Show Information"),
                             "%s!type" % (self._prefix),
                             options, NOTE_INFORMATION)
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        None
