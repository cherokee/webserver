import validations

from Page import *
from Table import *
from Entry import *
from Form import *

DATA_VALIDATION = [
    ("server!mime_files", validations.is_path_list),
]

COMMENT = """
<p>Here you can define which MIME types files you want Cherokee to
query when it serves static content.</p>
"""

class PageMime (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'mime', cfg)
        FormHelper.__init__ (self, 'mime', cfg)

    def _op_render (self):
        content = self._render_icon_list()

        self.AddMacroContent ('title', 'Mime types configuration')
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_apply_changes (self, uri, post):
        self.ApplyChanges ([], post, DATA_VALIDATION)
        return "/%s" % (self._id)

    def _render_icon_list (self):
        txt  = '<h1>MIME types</h1>'        
        txt += self.Dialog(COMMENT)

        table = Table(2, 1)
        self.AddTableEntry (table, 'Files', 'server!mime_files')
        txt += self.Indent(table)

        form = Form ('/%s' % (self._id))
        return form.Render(txt,DEFAULT_SUBMIT_VALUE)
