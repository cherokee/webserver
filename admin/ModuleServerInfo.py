from Form import *
from Table import *
from ModuleHandler import *

NOTE_INFORMATION = 'Which information should be shown.'

options = [
    ('normal',             "Server Information"),
    ('just_about',         "Only version information"),
    ('connection_details', "Server Information + Connections")
]

HELPS = [
    ('modules_handlers_server_info', "Server Information")
]

class ModuleServerInfo (ModuleHandler):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'server_info', cfg, prefix, submit_url)
        self.show_document_root = False

    def _op_render (self):
        txt  = "<h2>Privacy settings</h2>"

        table = TableProps()
        self.AddPropOptions_Reload (table, "Show Information",
                             "%s!type" % (self._prefix),
                             options, NOTE_INFORMATION)
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        None
