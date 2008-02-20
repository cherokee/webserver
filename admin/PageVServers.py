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
a custom number of parameter and rules that have to be apply to one or
more domains.</p>
"""

class PageVServers (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'vserver', cfg)
        FormHelper.__init__ (self, 'vserver', cfg)

    def _op_render (self):
        content = self._render_vserver_list()

        self.AddMacroContent ('title', 'Virtual Servers configuration')
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_handler (self, uri, post):
        if uri.startswith('/add_vserver'):
            tmp = self._op_add_vserver (post)
            if self.has_errors():
                return self._op_render()
            return tmp
        raise 'Unknown method'
            
    def _render_vserver_list (self):        
        vservers = self._cfg['vserver']
        txt = "<h1>Virtual Servers</h1>"
        txt += self.Dialog (COMMENT)
        
        # Render Virtual Server list
        table = Table(2)
        if vservers:
            txt += "<h3>Virtual Server List</h3>"
        
            for vserver in vservers:
                link = '<a href="/vserver/%s">%s</a>' % (vserver, vserver)
                if vserver != "default":
                    js = "post_del_key('/vserver/ajax_update', 'vserver!%s');"%(vserver)
                    link_del = self.InstanceButton ("Del", onClick=js)
                else:
                    link_del = ''
                table += (link, link_del)
            txt += self.Indent(table)

        # Add new Virtual Server
        table = Table(3,1)
        table += ('Name', 'Document Root')
        fo1 = Form ("/%s/add_vserver" % (self._id), add_submit=False)
        en1 = self.InstanceEntry ("new_vserver_name", "text")
        en2 = self.InstanceEntry ("new_vserver_droot", "text")
        table += (en1, en2, SUBMIT_ADD)

        txt += "<h3>Add new Virtual Server</h3>"
        txt += fo1.Render(str(table))

        return txt

    def _op_add_vserver (self, post):
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        name  = post['new_vserver_name'][0]
        droot = post['new_vserver_droot'][0]
        pre   = 'vserver!%s' % (name)

        # Do not add the server if it already exists
        if name in self._cfg['vserver']:
            return '/vserver'

        self._cfg['%s!document_root' % (pre)] = droot
        self._cfg['%s!directory!/!handler' % (pre)]  = "common"
        self._cfg['%s!directory!/!priority' % (pre)] = "1"

        return '/vserver/%s' % (name)
