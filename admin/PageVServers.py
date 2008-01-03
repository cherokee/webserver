from Page import *
from Form import *
from Table import *
from Entry import *

class PageVServers (PageMenu):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, cfg)
        self._id = 'vserver'

    def _op_render (self):
        content = self._render_vserver_list()

        self.AddMacroContent ('title', 'Virtual Servers configuration')
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_handler (self, uri, post):
        if uri.startswith('/add_vserver'):
            return self._op_add_vserver (post)
        raise 'Unknown method'
            
    def _render_vserver_list (self):        
        vservers = self._cfg['vserver']
        txt = "<h1>Virtual Servers</h1>"
        
        # Render Virtual Server list
        table = Table(2)
        if vservers:
            for vserver in vservers:
                link = '<a href="/vserver/%s">%s</a>' % (vserver, vserver)
                link_del = '<a href="/vserver/%s/remove">Remove</a>' % (vserver)
                table += (link, link_del)
            txt += str(table)

        # Add new Virtual Server
        table = Table(3,1)
        table += ('Name', 'Document Root')
        fo1 = Form ("/%s/add_vserver" % (self._id), add_submit=False)
        en1 = Entry ("new_vserver_name", "text")
        en2 = Entry ("new_vserver_droot", "text")
        table += (en1, en2, SUBMIT_ADD)

        txt += "<h3>Add new Virtual Server</h3>"
        txt += fo1.Render(str(table))

        return txt

    def _op_add_vserver (self, post):
        name  = post['new_vserver_name'][0]
        droot = post['new_vserver_droot'][0]
        pre   = 'vserver!%s' % (name)

        self._cfg['%s!document_root' % (pre)] = droot
        self._cfg['%s!directory!/!handler' % (pre)]  = "common"
        self._cfg['%s!directory!/!priority' % (pre)] = "100"

        return '/vserver/%s' % (name)
