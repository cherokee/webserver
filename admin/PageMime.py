import validations
from urllib import quote

from Page import *
from Table import *
from Entry import *
from Form import *

DATA_VALIDATION = [
    ("mime!.*!extensions", validations.is_extension_list),
    ("mime!.*!max\-age",   validations.is_number),
    ("tmp!new_extensions", validations.is_extension_list),
    ("tmp!new_maxage",     validations.is_number)
]

HELPS = [('config_mime_types', "MIME types")]

NOTE_NEW_MIME       = 'New MIME type to be added.'
NOTE_NEW_EXTENSIONS = 'Comma separated list of file extensions associated with the MIME type.'
NOTE_NEW_MAXAGE     = 'Maximum time that this sort of content can be cached (in seconds).'

TABLE_JS = """
<script type="text/javascript">
     $(document).ready(function() {
        $("#mimes tr:even').addClass('alt')");
        $("table.rulestable tr:odd").addClass("odd");
     });
</script>
"""

class PageMime (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'mime', cfg, HELPS)
        FormHelper.__init__ (self, 'mime', cfg)

    def _op_render (self):
        content = self._render_content()

        self.AddMacroContent ('title', 'Mime types configuration')
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_apply_changes (self, uri, post):
        if post.get_val('tmp!new_mime') and \
          (post.get_val('tmp!new_extensions') or
           post.get_val('tmp!new_maxage')):
            self._add_new_mime (post)
            if self.has_errors():
                return self._op_render()

        self.ApplyChanges ([], post, DATA_VALIDATION)
        return "/%s" % (self._id)

    def _add_new_mime (self, post):
        # Validate entries
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return
        
        mime = post.pop('tmp!new_mime')
        exts = post.pop('tmp!new_extensions')
        mage = post.pop('tmp!new_maxage')

        if mage:
            self._cfg['mime!%s!max-age'%(mime)] = mage
        if exts:
            self._cfg['mime!%s!extensions'%(mime)] = exts

    def _render_content (self):
        render = '<h1>MIME types</h1>'

        content = self._render_mime_list()
        form = Form ('/%s' % (self._id), add_submit=False)
        tmp = form.Render (content, DEFAULT_SUBMIT_VALUE)
        render += self.Indent(tmp)

        render += '<h2>Add new MIME</h2>\n'

        content = self._render_add_mime()
        form = Form ('/%s' % (self._id), add_submit=True, auto=False)
        tmp = form.Render (content, DEFAULT_SUBMIT_VALUE)
        render += self.Indent(tmp)

        return render

    def _render_mime_list (self):
        txt = ''
        cfg = self._cfg['mime']
        if cfg:
            table = Table(4, 1, style='id="mimes" class="rulestable"')
            table += ('Mime type', 'Extensions', 'MaxAge<br/>(<i>secs</i>)')
            keys = cfg.keys()
            keys.sort()
            for mime in keys:
                cfg_key = 'mime!%s'%(mime)
                e1 = self.InstanceEntry('%s!extensions'%(cfg_key), 'text', size=35)
                e2 = self.InstanceEntry('%s!max-age'%(cfg_key),    'text', size=6, maxlength=6)
                js = "post_del_key('/ajax/update', '%s');" % (quote(cfg_key))
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (mime, e1, e2, link_del)
            txt += '<div id="mimetable">%s</div>'%(str(table))
            txt += TABLE_JS
            txt += '<p><br /></p>'
        return txt

    def _render_add_mime (self):
        table = TableProps()

        self.AddPropEntry (table, 'Mime Type',  'tmp!new_mime',       NOTE_NEW_MIME)
        self.AddPropEntry (table, 'Extensions', 'tmp!new_extensions', NOTE_NEW_EXTENSIONS)
        self.AddPropEntry (table, 'Max Age',    'tmp!new_maxage',     NOTE_NEW_MAXAGE, maxlength=6)

        return str(table)

        e1 = self.InstanceEntry('tmp!new_mime',       'text', size=35)
        e2 = self.InstanceEntry('tmp!new_extensions', 'text', size=35)
        e3 = self.InstanceEntry('tmp!new_maxage',     'text', size=6, maxlength=6)

        table = Table(3,1, style='width="100%"')
        table += ('Mime Type', 'Extensions', 'Max Age')
        table += (e1, e2, e3)

        return str(table)
