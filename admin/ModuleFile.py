from Form import *
from Table import *
from ModuleHandler import *

NOTE_IO_CACHE = 'Enables an internal I/O cache that improves performance.'

HELPS = [
    ('modules_handlers_file', "Static Content")
]

class ModuleFile (ModuleHandler):
    PROPERTIES = [
        'iocache'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'file', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''

        table = TableProps()
        self.AddPropCheck (table, "Use I/O cache", "%s!iocache" % (self._prefix), True, NOTE_IO_CACHE)

        txt += '<h2>File Sending</h2>'
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['iocache'], post)

