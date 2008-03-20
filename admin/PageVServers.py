import validations

from Page import *
from Form import *
from Table import *
from Entry import *

DATA_VALIDATION = [
    ("new_vserver_name",  validations.is_safe_id),
    ("new_vserver_droot", validations.is_local_dir_exists),
]

COMMENT = """
<p>'Virtual Server' is an abstraction mechanism that allows to define
a custom number of parameters and rules that have to be applied to one or
more domains.</p>
"""

class PageVServers (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'vservers', cfg)
        FormHelper.__init__ (self, 'vservers', cfg)

    def _op_render (self):
        content = self._render_vserver_list()

        self.AddMacroContent ('title', 'Virtual Servers configuration')
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_handler (self, uri, post):
        if post.get_val('is_submit'):
            tmp = self._op_add_vserver (post)
            if self.has_errors():
                return self._op_render()
        return self._op_render()

    def _render_vserver_list (self):        
        vservers = self._cfg['vserver']
        txt = "<h1>Virtual Servers</h1>"
        txt += self.Dialog (COMMENT)
        
        # Render Virtual Server list
        if vservers: 
            txt += "<h2>Virtual Server List</h2>"

            table = Table(4, style='width="100%"')
            table += ('Name', 'Document Root', 'Logging', '')

            for vserver in vservers:
                document_root = self._cfg.get_val('vserver!%s!document_root'%(vserver), '')
                logger_val    = self._cfg.get_val('vserver!%s!logger'%(vserver))

                if logger_val:
                    logging = 'yes'
                else:
                    logging = 'no'

                link = '<a href="/vserver/%s">%s</a>' % (vserver, vserver)
                if vserver != "default":
                    js = "post_del_key('/ajax/update', 'vserver!%s');"%(vserver)
                    link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                else:
                    link_del = ''
                table += (link, document_root, logging, link_del)
            txt += self.Indent(table)

        # Add new Virtual Server
        table = Table(3,1)
        table += ('Name', 'Document Root')
        fo1 = Form ("/vserver", add_submit=False)
        en1 = self.InstanceEntry ("new_vserver_name", "text")
        en2 = self.InstanceEntry ("new_vserver_droot", "text")
        table += (en1, en2, SUBMIT_ADD)

        txt += "<h2>Add new Virtual Server</h2>"
        txt += fo1.Render(str(table))

        return txt

    def _op_add_vserver (self, post):
        # Ensure that no entry in empty
        for key in ['new_vserver_name', 'new_vserver_droot']:
            if not post.get_val(key):
                self._error_add (key, '', 'Cannot be empty')

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        name  = post.pop('new_vserver_name')
        droot = post.pop('new_vserver_droot')
        pre   = 'vserver!%s' % (name)

        # Do not add the server if it already exists
        if name in self._cfg['vserver']:
            return '/vserver'

        self._cfg['%s!document_root' % (pre)] = droot
        self._cfg['%s!directory!/!handler' % (pre)]  = "common"
        self._cfg['%s!directory!/!priority' % (pre)] = "1"

        return '/vserver/%s' % (name)
