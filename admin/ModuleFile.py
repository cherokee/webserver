from Form import *
from Table import *
from ModuleHandler import *

NOTE_IO_CACHE = _('Enables an internal I/O cache that improves performance.')

HELPS = [
    ('modules_handlers_file', _("Static Content"))
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
        self.AddPropCheck (table, _("Use I/O cache"), "%s!iocache" % (self._prefix), True, NOTE_IO_CACHE)

        txt += '<h2>%s</h2>' % (_('File Sending'))
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['iocache'], post)

