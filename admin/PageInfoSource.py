import validations

from Page import *
from Table import *
from Entry import *
from Form import *
from Rule import *

# For gettext
N_ = lambda x: x

NOTE_SOURCE      = N_('The source can be either a local interpreter or a remote host acting as an information source.')
NOTE_NICK        = N_('Source nick. It will be referenced by this name in the rest of the server.')
NOTE_TYPE        = N_('It allows to choose whether it runs the local host or a remote server.')
NOTE_HOST        = N_('Where the information source can be accessed. The host:port pair, or the Unix socket path.')
NOTE_INTERPRETER = N_('Command to spawn a new source in case it were not accessible.')
NOTE_TIMEOUT     = N_('How long should the server wait when spawning an interpreter before giving up (in seconds). Default: 3.')
NOTE_USAGE       = N_('Sources currently in use. Note that the last source of any rule cannot be deleted until the rule has been manually edited.')
NOTE_USER        = N_('Execute the interpreter under a different user. Default: Same UID as the server.')
NOTE_GROUP       = N_('Execute the interpreter under a different group. Default: Default GID of the new process UID.')

TABLE_JS = """
<script type="text/javascript">
     $(document).ready(function() {
        $("#sources tr:even').addClass('alt')");
        $("table.rulestable tr:odd").addClass("odd");
     });
</script>
"""

HELPS = [
    ('config_info_sources', N_("Information Sources"))
]

DATA_VALIDATION = [
    ('source!.+?!host',        validations.is_information_source),
    ('source!.+?!timeout',     validations.is_positive_int),
    ('tmp!new_source_host',    validations.is_information_source),
    ('tmp!new_source_timeout', validations.is_positive_int)
]

RULE_NAME_LEN_LIMIT = 35

class PageInfoSource (PageMenu, FormHelper):
    def __init__ (self, cfg):
        FormHelper.__init__ (self, 'source', cfg)
        PageMenu.__init__ (self, 'source', cfg, HELPS)
        self.submit_url = '/source/'

    def _op_handler (self, uri, post):
        if uri.endswith('/ajax_update'):
            used_sources = self._get_used_sources()
            for src in post:
                self._cfg.pop(src)
                src = src.split('!')[1]
                if not used_sources.has_key(src):
                    continue
                for r in used_sources[src]:
                    self._cfg.pop(r)

        if post.get_val('is_submit'):
            if post.get_val('tmp!new_source_nick'):
                return self._apply_new_source (uri, post)
            else:
                source = post.pop('source_num')

                if (post.get_val ('new_env_name') and
                    post.get_val ('new_env_value')):
                    self._apply_add_new_env_var(post, source)

                self.ApplyChanges ([], post, validation = DATA_VALIDATION)
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
        self.AddMacroContent ('title', _('Information Sources'))
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _apply_add_new_env_var (self, post, source):
        name  = post.pop ('new_env_name')
        value = post.pop ('new_env_value')

        self._cfg['source!%s!env!%s' % (source, name)] = value

    def _apply_new_source (self, uri, post):
        self.ValidateChange_SingleKey ('tmp!new_source_timeout', post, DATA_VALIDATION)
        self.ValidateChange_SingleKey ('tmp!new_source_host',    post, DATA_VALIDATION)
        if self.has_errors():
            return self._op_render()

        nick  = post.pop ('tmp!new_source_nick')
        type  = post.pop ('tmp!new_source_type')
        host  = post.pop ('tmp!new_source_host')
        inter = post.pop ('tmp!new_source_interpreter')
        time  = post.pop ('tmp!new_source_timeout')
        user  = post.pop ('tmp!new_source_user')
        group = post.pop ('tmp!new_source_group')

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
        self._cfg['source!%d!timeout'%(prio)]     = time
        self._cfg['source!%d!user'%(prio)]        = user
        self._cfg['source!%d!group'%(prio)]       = group

        return '/%s/%d' % (self._id, prio)

    def _render_source_details_env (self, s):
        txt = ''
        envs = self._cfg.keys('source!%s!env'%(s))
        if envs:
            txt += '<h3>%s</h3>' % (_('Environment variables'))
            table = Table(3, title_left=1, style='width="90%"')

            for env in envs:
                pre = 'source!%s!env!%s'%(s,env)
                val = self.InstanceEntry(pre, 'text', size=25)
                link_del = self.AddDeleteLink ('/ajax/update', pre)
                table += (env, val, link_del)

            txt += self.Indent(table)
            txt += self.HiddenInput ('source_num', s)

        fo = Form ("/%s"%(self._id), add_submit=False, auto=True)
        render=fo.Render(txt)

        txt = '<h3>%s</h3>' % (_('Add new Environment variable'))
        name  = self.InstanceEntry('new_env_name',  'text', size=25)
        value = self.InstanceEntry('new_env_value', 'text', size=25)

        table = Table(3, 1, style='width="90%"')
        table += (_('Variable'), _('Value'), '')
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
        self.AddPropOptions_Reload (table, _('Type'),'source!%s!type'%(s), SOURCE_TYPES, _(NOTE_TYPE))
        self.AddPropEntry   (table, _('Nick'),       'source!%s!nick'%(s), _(NOTE_NICK), req=True)
        self.AddPropEntry   (table, _('Connection'), 'source!%s!host'%(s), _(NOTE_HOST), req=True)
        if type == 'interpreter':
            self.AddPropEntry (table, _('Interpreter'),      'source!%s!interpreter'%(s),  _(NOTE_INTERPRETER), req=True)
            self.AddPropEntry (table, _('Spawning timeout'), 'source!%s!timeout'%(s), _(NOTE_TIMEOUT))
            self.AddPropEntry (table, _('Execute as User'),  'source!%s!user'%(s), _(NOTE_USER))
            self.AddPropEntry (table, _('Execute as Group'), 'source!%s!group'%(s), _(NOTE_GROUP))

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
        self.AddPropOptions_Reload (table, _('Type'),       'tmp!new_source_type', SOURCE_TYPES, _(NOTE_TYPE))
        self.AddPropEntry          (table, _('Nick'),       'tmp!new_source_nick', _(NOTE_NICK), req=True)
        self.AddPropEntry          (table, _('Connection'), 'tmp!new_source_host', _(NOTE_HOST), req=True)
        if type == 'interpreter' or not type:
            self.AddPropEntry (table, _('Interpreter'),      'tmp!new_source_interpreter', _(NOTE_INTERPRETER), req=True)
            self.AddPropEntry (table, _('Spawning timeout'), 'tmp!new_source_timeout', _(NOTE_TIMEOUT))
            self.AddPropEntry (table, _('Execute as User'),  'tmp!new_source_user', _(NOTE_USER))
            self.AddPropEntry (table, _('Execute as Group'), 'tmp!new_source_group', _(NOTE_GROUP))

        txt += self.Indent(table)
        return txt


    def _render_content (self, source):
        title = _('Information Sources Settings')
        if source:
            nick = self._cfg.get_val('source!%s!nick'%(source))
            txt = "<h1>%s: %s</h1>" % (title, nick)
        else:
            txt = "<h1>%s</h1>" % (title)

        # List
        #
        if self._cfg.keys('source'):
            protect = self._get_protected_list()

            txt += "<h2>%s</h2>" % (_('Known sources'))
            table  = '<table width="90%" id="sources" class="rulestable">'
            table += '<tr><th>%s</th><th>%s</th><th>%s</th><th></th></tr>' % \
                     (_('Nick'), _('Type'), _('Connection'))

            keys = self._cfg.keys('source')
            keys.sort()
            for s in keys:
                nick = self._cfg.get_val('source!%s!nick'%(s))
                host = self._cfg.get_val('source!%s!host'%(s))
                type = self._cfg.get_val('source!%s!type'%(s))

                if s in protect:
                    msg = _("Deletion forbidden. Check source usage.")
                    link = self.InstanceImage ("forbidden.png", msg, border="0")
                else:
                    link = self.AddDeleteLink ('/source/ajax_update', 'source!%s'%(s))

                table += '<tr><td><a href="/%s/%s">%s</td><td>%s</td><td>%s</td><td>%s</td></tr>' % (self._id, s, nick, type, host, link)
            table += '<tr><th colspan="4"><a href="/%s"><div align="center">%s</div></a></th></tr>' % (self._id, _('Add new'))
            table += '</table>'
            txt += self.Indent(table)
            txt += TABLE_JS

        if (source):
            # Details
            #
            nick = self._cfg.get_val('source!%s!nick'%(source))
            txt += "<h2>%s '%s'</h2>" % (_('Details:'), nick)
            txt += self.Indent(self._render_source_details (source))

        else:
            # Add new
            #
            tmp = "<h2>%s</h2>" % (_('Add a new'))
            tmp += self._render_add_new()

            fo1 = Form ("/%s"%(self._id), add_submit=False)
            txt += fo1.Render(tmp)

        # List info about source usage
        #
        if self._cfg.keys('source'):
            txt += "<h2>%s</h2>" % (_('Source usage'))
            txt += self.Indent(self.Dialog (NOTE_USAGE))
            txt += self._render_source_usage ()

        return txt


    def _render_source_usage (self):
        """List the usage of each information source"""
        used_sources = self._get_used_sources()
        table  = '<table width="90%" id="usage" class="rulestable">'
        table += '<tr><th>%s</th><th>%s</th><th>%s</th></tr>' % \
                 (_('Nick'), _('Virtual server'), _('Rule'))

        keys = used_sources.keys()
        keys.sort()
        for src in keys:
            for entry in used_sources[src]:
                is_user_dir = False

                tmp = entry.split('!')
                if tmp[2] == 'user_dir':
                    is_user_dir = True
                    serv_id = tmp[1]
                    rule_id = tmp[4]
                    rule = 'vserver!%s!user_dir!rule!%s'%(serv_id, rule_id)
                    rule_url = '/vserver/%s/userdir/rule/%s'%(serv_id, rule_id)
                else:
                    serv_id = tmp[1]
                    rule_id = tmp[3]
                    rule = 'vserver!%s!rule!%s'%(serv_id, rule_id)
                    rule_url = '/vserver/%s/rule/%s'%(serv_id, rule_id)

                nick = self._cfg.get_val('source!%s!nick'%(src))
                serv = self._cfg.get_val('vserver!%s!nick'%(serv_id))

                # Try to get the rule name
                rule = Rule(self._cfg, '%s!match'%(rule), self.submit_url, self.errors, 0)
                rule_name = rule.get_name()

                if len(rule_name) > RULE_NAME_LEN_LIMIT:
                    rule_name = "%s<b>...</b>" % (rule_name[:RULE_NAME_LEN_LIMIT])

                nick_td = '<td><a href="/%s/%s">%s</td>'%(self._id, src, nick)
                serv_td = '<td><a href="/vserver/%s">%s</a></td>'%(serv_id, serv)

                if is_user_dir:
                    rule_td = '<td><a href="%s">User Dir: %s</a></td>'%(rule_url, rule_name)
                else:
                    rule_td = '<td><a href="%s">%s</a></td>'%(rule_url, rule_name)

                table += '<tr>%s%s%s</tr>'%(nick_td, serv_td, rule_td)

        table += '</table>'

        txt  = ''
        txt += self.Indent(table)
        txt += TABLE_JS

        return txt

    def _get_used_sources (self):
        """List of every rule using any info source"""
        used_sources = {}
        target ='!balancer!source!'
        for entry in self._cfg.serialize().split():
            if target in entry:
                source = self._cfg.get_val(entry)
                if not used_sources.has_key(source):
                    used_sources[source] = []
                used_sources[source].append(entry)
        return used_sources

    def _get_protected_list (self):
        """List of sources to protect against deletion"""
        rule_sources = {}
        used_sources = self._get_used_sources()
        for src in used_sources:
            for r in used_sources[src]:
                rule = r.split('!handler!balancer!')[0]
                if not rule_sources.has_key(rule):
                    rule_sources[rule] = []
                rule_sources[rule].append(src)

        protect = []
        for s in rule_sources:
            if len(rule_sources[s])==1:
                protect.append(rule_sources[s][0])
        return protect
