from Form import *
from Table import *
from Module import *

NOTE_IO_CACHE = 'Enables an internal I/O cache that improves performance.'

class ModuleFile (Module, FormHelper):
    PROPERTIES = [
        'iocache'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        Module.__init__ (self, 'file', cfg, prefix, submit_url)
        FormHelper.__init__ (self, 'file', cfg)

    def _op_render (self):
        txt = ''

        table = TableProps()
        self.AddPropCheck (table, "Use I/O cache", "%s!iocache" % (self._prefix), True, NOTE_IO_CACHE)

        txt += '<h2>File Sending</h2>'
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['iocache'], post)

