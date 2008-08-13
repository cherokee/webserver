from Form import *
from Table import *
from Module import *

NOTE_INFORMATION = 'Which information should be shown.'

options = [
    ('normal',             "Server Information"),
    ('just_about',         "Only version information"),
    ('connection_details', "Server Information + Connections")
]

class ModuleServerInfo (Module, FormHelper):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'server_info', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'server_info', cfg)

    def _op_render (self):
        txt  = "<h2>Privacy settings</h2>"

        table = TableProps()
        self.AddPropOptions (table, "Show Information", 
                             "%s!type" % (self._prefix),
                             options, NOTE_INFORMATION)
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        None
