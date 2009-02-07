from Form import *
from Table import *
from ModuleHandler import *

NOTE_RATE        = 'Figure the bit rate of the media file, and limit the bandwidth to it.'
NOTE_RATE_FACTOR = 'Factor to increases the bandwidth limit. Default: 0.01.'
NOTE_RATE_BOOST  = 'Number of seconds to stream before setting the bandwidth limit. Default: 5.'

HELPS = [
    ('modules_handlers_streaming', "Audio/Video Streaming")
]

class ModuleStreaming (ModuleHandler):
    PROPERTIES = [
        'rate'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'streaming', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''

        table = TableProps()
        self.AddPropCheck (table, "Auto Rate",      "%s!rate" % (self._prefix), True,  NOTE_RATE)
        if int(self._cfg.get_val ('%s!rate'%(self._prefix), "1")):
            self.AddPropEntry (table, "Multity Factor", "%s!rate_factor" % (self._prefix), NOTE_RATE_FACTOR)
            self.AddPropEntry (table, "Initial Boost",  "%s!rate_boost" % (self._prefix),  NOTE_RATE_BOOST)

        txt += '<h2>Audio/Video Streaming</h2>'
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, ['rate'], post)

