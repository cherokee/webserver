import validations

from Page import *
from Table import *
from Entry import *
from Form import *
from GraphManager import *

# For gettext
N_ = lambda x: x

TITLE           = N_("Graphs and Statistics")
NOTE_COLLECTORS = N_('How the usage graphics should be generated.')
NOTE_GRAPHS     = N_("Choose the graph information you'd like to see.")
NOTE_NO_VSRV    = N_("None of the Virtual Servers is enabled collect information.")

DATA_VALIDATION = []

GRAPHS_SERVER = [
    ('traffic',     "Global Server Traffic"),
    ('connections', "Accepted Connections"),
    ('timeouts',    "Connection Timeouts")
]


class PageStats (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'stats', cfg, [])
        FormHelper.__init__ (self, 'stats', cfg)
        self.set_submit_url ("/%s/"%(self._id))

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', _(TITLE))
        self.AddMacroContent ('content', content)
        return Page.Render(self)
    
    def _render_content (self):
        txt = "<h1>%s</h1>" % (_(TITLE))

        tabs = []
        if self._cfg.get_val ('server!collector'):
            tabs += [(_('Server'),          self._render_server_graphs())]
            tabs += [(_('Virtual Servers'), self._render_vservers_graphs())]
        tabs += [(_('Config'),          self._render_config())]

        txt += self.InstanceTab (tabs)
        form = Form ("/%s" % (self._id), add_submit=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _render_config (self):
        txt = ''

        txt += "<h2>%s</h2>" % (_('Information Collector'))
        table = TableProps()
        e = self.AddPropOptions_Reload (table, _('Graphs Type'), 'server!collector',
                                        modules_available(COLLECTORS), _(NOTE_COLLECTORS))
        txt += self.Indent(str(table) + e)
        return txt

    def _render_server_graphs (self):
        txt = '<h2>Server Information</h2>'

        table = TableProps()
        self.AddPropOptions_Reload (table, _('Graphs'), 'tmp!server_graph', 
                                    GRAPHS_SERVER, _(NOTE_GRAPHS))
        txt += self.Indent(table)

        graph = self._cfg.get_val('tmp!server_graph', GRAPHS_SERVER[0][0])
        if graph == "traffic":
            for tmp in graphs_get_images (self._cfg, "server_traffic"):
                interval, img_name = tmp
                txt += '<p><img src="/graphs/%s" alt="Traffic %s" /></p>\n' % (img_name, interval)
        elif graph == "connections":
            for tmp in graphs_get_images (self._cfg, "server_accepts"):
                interval, img_name = tmp
                txt += '<p><img src="/graphs/%s" alt="Accepts %s" /></p>\n' % (img_name, interval)
        elif graph == "timeouts":
            for tmp in graphs_get_images (self._cfg, "server_timeouts"):
                interval, img_name = tmp
                txt += '<p><img src="/graphs/%s" alt="Timeouts %s" /></p>\n' % (img_name, interval)

        return txt

    def _render_vservers_graphs (self):
        txt = '<h2>Virtual Servers Information</h2>'

        # Virtual Servers
        vsrvs = []
        for v in self._cfg.keys("vserver"):
            enabled = int(self._cfg.get_val("vserver!%s!collector!enabled"%(v), "1"))
            if enabled:
                nick = self._cfg.get_val('vserver!%s!nick'%(v))
                vsrvs.append((v, nick))

        if not len(vsrvs):
            txt += self.Dialog(NOTE_NO_VSRV)
            return txt

        # Selection table
        table = TableProps()
        self.AddPropOptions_Reload (table, _('Server'), 'tmp!vserver_choosen', 
                                    vsrvs, _(NOTE_GRAPHS))
        txt += self.Indent(table)

        # Graphs
        vsrv = self._cfg.get_val("tmp!vserver_choosen", vsrvs[0][0])
        name = self._cfg.get_val('vserver!%s!nick'%(vsrv))

        for tmp in graphs_get_images (self._cfg, "vserver_traffic_%s"%(name)):
            interval, img_name = tmp
            txt += '<p><img src="/graphs/%s" alt="%s, Traffic %s" /></p>\n' % (img_name, name, interval)

        return txt

    def _op_apply_changes (self, uri, post):
        # Validation
        validation = DATA_VALIDATION[:]

        pre  = "server!collector"
        name = self._cfg.get_val (pre)

        if name:
            graph_module = module_obj_factory (name, self._cfg, pre, self.submit_url)
            if 'validation' in dir(graph_module):
                validation += graph_module.validation

        self.ApplyChanges ([], post, validation)


        
