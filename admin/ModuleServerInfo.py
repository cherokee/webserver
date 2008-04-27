from Form import *
from Table import *
from Module import *

NOTE_JUST_ABOUT  = 'Show only the most basic information.'
NOTE_CONNECTIONS = 'Show status of the ongoing connections.'

class ModuleServerInfo (Module, FormHelper):
    PROPERTIES = [
        'just_about',
        'connection_details'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'server_info', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'server_info', cfg)

    def _op_render (self):
        txt  = "<h2>Privacy settings</h2>"

        table = TableProps()
        self.AddPropCheck (table, "Just 'About'", "%s!just_about" % (self._prefix), False, NOTE_JUST_ABOUT)
        self.AddPropCheck (table, "Show Connections", "%s!connection_details" % (self._prefix), False, NOTE_CONNECTIONS)
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['just_about'], post)
