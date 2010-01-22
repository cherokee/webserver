from consts import *
from Form import *
from Table import *
from ModuleHandler import *

HELPS = [
    ('modules_handlers_post_report', _("POST Report"))
]

NOTE_LANGUAGES = "Target language for which the information will be encoded. (Default: JSON)"


class ModulePostReport (ModuleHandler):
    PROPERTIES = []

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'post_report', cfg, prefix, submit_url)
        self.show_document_root = False

    def _op_render (self):
        txt  = "<h2>%s</h2>" % (_('Formatting options'))

        table = TableProps()
        self.AddPropOptions_Reload_Plain (table, _("Target language"),
                                          "%s!lang" % (self._prefix),
                                          DWRITER_LANGS, _(NOTE_LANGUAGES))
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        if not self._cfg.get_val('server!post_track'):
            self._cfg['server!post_track'] = 'post_track'

