import validations

from Page import *
from Table import *
from Entry import *
from Form import *

NOTE_SOURCE      = 'The source can be either a local interpreter or a remote host acting as an information source.'
NOTE_NICK        = 'Source nick. It will be referenced by this name in the rest of the server.'
NOTE_TYPE        = 'It allows to choose whether it runs the local host or a remote server.'
NOTE_HOST        = 'Where the information source can be accessed. The host:port pair, or the Unix socket path.'
NOTE_INTERPRETER = 'Command to spawn a new source in case it were not accessible.'


TABLE_JS = """
<script type="text/javascript">
     $(document).ready(function() {
        $("#sources tr:even').addClass('alt')");
        $("table.rulestable tr:odd").addClass("odd");
     });
</script>
"""

HELPS = [
    ('config_info_sources', "Info Sources")
]

class PageInfoSource (PageMenu, FormHelper):
    def __init__ (self, cfg):
        FormHelper.__init__ (self, 'source', cfg)
        PageMenu.__init__ (self, 'source', cfg, HELPS)
        self.submit_url = '/source/'

    def _op_handler (self, uri, post):
        if post.get_val('is_submit'):
            if post.get_val('tmp!new_source_nick'):
                return self._apply_new_source (uri, post)
            else:
                source = post.pop('source_num')

                if (post.get_val ('new_env_name') and
                    post.get_val ('new_env_value')):
                    self._apply_add_new_env_var(post, source)

                self.ApplyChanges ([], post)
                return "/%s/%s" % (self._id, source)

        tmp = uri.split('/')
        if len(tmp) >= 2:
            source = tmp[1]
            if not self._cfg.keys('source!%s'%(source)):
                return '/%s' % (self._id)
            return self._op_render (source)
        self._op_render()

    def _op_render (self, source=None):
        content = self._render_content (source)
        self.AddMacroContent ('title', 'Information Sources')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _apply_add_new_env_var (self, post, source):
        name  = post.pop ('new_env_name')
        value = post.pop ('new_env_value')

        self._cfg['source!%s!env!%s' % (source, name)] = value

    def _apply_new_source (self, uri, post):
        nick  = post.pop ('tmp!new_source_nick')
        type  = post.pop ('tmp!new_source_type')
        host  = post.pop ('tmp!new_source_host')
        inter = post.pop ('tmp!new_source_interpreter')

        tmp = [int(x) for x in self._cfg.keys('source')]
        tmp.sort()
        if tmp:
            prio = tmp[-1] + 1
        else:
            prio = 1

        self._cfg['source!%d!nick'%(prio)]        = nick
        self._cfg['source!%d!type'%(prio)]        = type
        self._cfg['source!%d!host'%(prio)]        = host
        self._cfg['source!%d!interpreter'%(prio)] = inter

        return '/%s/%d' % (self._id, prio)

    def _render_source_details_env (self, s):
        txt = ''
        envs = self._cfg.keys('source!%s!env'%(s))
        if envs:
            txt += '<h3>Environment variables</h3>'
            table = Table(3, title_left=1, style='width="90%"')
            for env in envs:
                pre = 'source!%s!env!%s'%(s,env)
                val = self.InstanceEntry(pre, 'text', size=25)
                js = "post_del_key('/ajax/update', '%s');"%(pre)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (env, val, link_del)

            txt += self.Indent(table)
            txt += self.HiddenInput ('source_num', s)

        fo = Form ("/%s"%(self._id), add_submit=False, auto=True)
        render=fo.Render(txt)

        txt = '<h3>Add new Environment variable</h3>'
        name  = self.InstanceEntry('new_env_name',  'text', size=25)
        value = self.InstanceEntry('new_env_value', 'text', size=25)

        table = Table(3, 1, style='width="90%%"')
        table += ('Variable', 'Value', '')
        table += (name, value, SUBMIT_ADD)

        txt += self.Indent (table)
        txt += self.HiddenInput ('source_num', s)
        fo = Form ("/%s"%(self._id), add_submit=False, auto=False)

        render += fo.Render(txt)
        return render

    def _render_source_details (self, s):
        txt = ''
        nick = self._cfg.get_val('source!%s!nick'%(s))
        type = self._cfg.get_val('source!%s!type'%(s))

        # Properties
        table = TableProps()
        self.AddPropOptions_Reload (table, 'Type','source!%s!type'%(s), SOURCE_TYPES, NOTE_TYPE)
        self.AddPropEntry   (table, 'Nick',       'source!%s!nick'%(s), NOTE_NICK,req=True)
        self.AddPropEntry   (table, 'Connection', 'source!%s!host'%(s), NOTE_HOST,req=True)
        if type == 'interpreter':
            self.AddPropEntry (table, 'Interpreter', 'source!%s!interpreter'%(s), NOTE_INTERPRETER, req=True)

        tmp  = self.HiddenInput ('source_num', s)
        tmp += str(table)

        fo = Form ("/%s"%(self._id), add_submit=False, auto=True)
        txt = fo.Render(tmp)

        # Environment variables
        if type == 'interpreter':
            tmp = self._render_source_details_env (s)
            txt += self.Indent(tmp)

        return txt

    def _render_add_new (self):
        txt  = ''
        type = self._cfg.get_val('tmp!new_source_type')

        table = TableProps()
        self.AddPropOptions_Reload (table, 'Type',       'tmp!new_source_type', SOURCE_TYPES, NOTE_TYPE)
        self.AddPropEntry          (table, 'Nick',       'tmp!new_source_nick', NOTE_NICK, req=True)
        self.AddPropEntry          (table, 'Connection', 'tmp!new_source_host', NOTE_HOST, req=True)
        if type == 'interpreter' or not type:
            self.AddPropEntry (table, 'Interpreter', 'tmp!new_source_interpreter', NOTE_INTERPRETER, req=True)

        txt += self.Indent(table)
        return txt


    def _render_content (self, source):
        txt = "<h1>Information Sources Settings</h1>"

        # List
        #
        if self._cfg.keys('source'):
            txt += "<h2>Known sources</h2>"

            table  = '<table width="90%%" id="sources" class="rulestable">'
            table += '<tr><th>Nick</th><th>Type</th><th>Connection</th></tr>'
            for s in self._cfg.keys('source'):
                nick = self._cfg.get_val('source!%s!nick'%(s))
                host = self._cfg.get_val('source!%s!host'%(s))
                type = self._cfg.get_val('source!%s!type'%(s))

                js = "post_del_key('/ajax/update', 'source!%s');"%(s)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)

                table += '<tr><td><a href="/%s/%s">%s</td><td>%s</td><td>%s</td><td>%s</td></tr>' % (self._id, s, nick, type, host, link_del)
            table += '<tr><td colspan="4" align="center"><br/><a href="/%s">Add new</a></td></tr>' % (self._id)
            table += '</table>'
            txt += self.Indent(table)
            txt += TABLE_JS

        if (source):
            # Details
            #
            nick = self._cfg.get_val('source!%s!nick'%(source))
            txt += "<h2>Details: '%s'</h2>" % (nick)
            txt += self.Indent(self._render_source_details (source))

        else:
            # Add new
            #
            tmp = "<h2>Add a new</h2>"
            tmp += self._render_add_new()

            fo1 = Form ("/%s"%(self._id), add_submit=False)
            txt += fo1.Render(tmp)

        return txt
