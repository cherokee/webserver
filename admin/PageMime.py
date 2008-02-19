import validations

from Page import *
from Table import *
from Entry import *
from Form import *

DATA_VALIDATION = [
    ("server!mime_files", validations.is_path_list),
]

class PageMime (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'mime', cfg)
        FormHelper.__init__ (self, 'mime', cfg)

    def _op_handler (self, uri, post):
        if uri.startswith('/update'):
            return self._op_apply_changes(post)
        raise 'Unknown method'

    def _op_render (self):
        content = self._render_icon_list()

        self.AddMacroContent ('title', 'Mime types configuration')
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_apply_changes (self, post):
        self.ApplyChanges ([], post, DATA_VALIDATION)
        return "/%s" % (self._id)

    def _render_icon_list (self):
        table = Table(2, 1)
        self.AddTableEntry (table, 'Files', 'server!mime_files')

        txt  = '<h1>System-wide MIME types file</h1>'        
        txt += self.Indent(table)

        form = Form ('%s/update' % (self._id))
        return form.Render(txt)
