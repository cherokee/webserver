import validations

from Page import *
from Form import *
from Table import *
from Entry import *

DATA_VALIDATION = [
    ("new_vserver_name",   validations.is_safe_id),
    ("new_vserver_droot", (validations.is_local_dir_exists, 'cfg')),
]

COMMENT = """
<p>'Virtual Server' is an abstraction mechanism that allows to define
a custom number of parameters and rules that have to be applied to one or
more domains.</p>
"""

HELPS = [
    ('config_virtual_servers', "Virtual Servers")
]

def domain_cmp (d1, d2):
    d1s = d1.split('.')
    d2s = d2.split('.')

    if len(d1s) >= 2 and len(d2s) >= 2:
        if d1s[-2] == d2s[-2]:                    # Domain name
            if d1s[-1] == d2s[-1]:                # TLD
                skip = len(".".join(d2s[-2:]))
                return cmp(d1[:-skip],d2[:-skip])
            else:
                return cmp(d1s[-1], d2s[-1])
        else:
            return cmp(d1s[-2],d2s[-2])
    else:
        return cmp(d1,d2)

class PageVServers (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'vservers', cfg, HELPS)
        FormHelper.__init__ (self, 'vservers', cfg)

        self._normailze_vservers()

    def _op_render (self):
        content = self._render_vserver_list()

        self.AddMacroContent ('title', 'Virtual Servers configuration')
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_handler (self, uri, post):
        if post.get_val('is_submit'):
            if post.get_val('vserver_clone_trg'):
                tmp = self._op_clone_vserver (post)

            elif post.get_val('new_vserver_name') or \
                 post.get_val('new_vserver_droot'):
                tmp = self._op_add_vserver (post)

            if self.has_errors():
                return self._op_render()
            return "/vserver"
                
        elif uri.endswith('/ajax_update'):
            if post.get_val('update_prio'):
                tmp = post.get_val('prios').split(',')
                order = filter (lambda x: x, tmp)

                self._reorder_vservers (order)
                return "ok"

        return self._op_render()

    def _normailze_vservers (self):
        vservers = [int(x) for x in self._cfg['vserver'].keys()]
        vservers.sort ()
        
        # Rename all the virtual servers
        for vserver in self._cfg['vserver'].keys():
            self._cfg.rename('vserver!%s'%(vserver),
                             'vserver!OLD_%d'%(int(vserver)))
        
        # Add them again
        for i in range(len(vservers)):
            prio = (i+1) * 10
            self._cfg.rename ('vserver!OLD_%d'%(vservers[i]),
                              'vserver!%d'%(prio))

    def _reorder_vservers (self, changes):
        changes.reverse()

        # Rename all the virtual servers
        for vserver in self._cfg['vserver'].keys():
            self._cfg.rename ('vserver!%s'%(vserver),
                              'vserver!OLD_%d'%(int(vserver)))

        # Rebuild the list according to 'changes'
        n = 10
        for prio in changes:
            self._cfg.rename ('vserver!OLD_%s'%(prio),
                              'vserver!%d'%(n))
            n += 10

    def _render_vserver_list (self):        
        txt = "<h1>Virtual Servers</h1>"
        txt += self.Dialog (COMMENT)

        vservers = self._cfg['vserver']
        table_name = 'vserver_sortable_table'

        sorted_vservers = self._cfg['vserver'].keys()
        sorted_vservers.sort(reverse=True)

        txt += '<table id="%s" class="rulestable">' % (table_name)
        txt += '<tr NoDrag="1" NoDrop="1"><th>Nickname</th><th>Root</th><th>Domains</th><th>Logging</th><th></th></tr>'

        for prio in sorted_vservers:
            nick          = self._cfg.get_val('vserver!%s!nick'%(prio))
            document_root = self._cfg.get_val('vserver!%s!document_root'%(prio), '')
            logger_val    = self._cfg.get_val('vserver!%s!logger'%(prio))
            domains       = self._cfg.keys('vserver!%s!domain'%(prio))

            if not domains:
                doms = 1
            else:
                doms = len(domains)
                if not nick in domains:
                    doms += 1

            link = '<a href="/vserver/%s">%s</a>' % (prio, nick)
            if nick == 'default':
                extra = ' NoDrag="1" NoDrop="1"'
            else:
                extra = ''

            if logger_val:
                logging = 'yes'
            else:
                logging = 'no'

            if nick != "default":
                js = "post_del_key('/ajax/update', 'vserver!%s');"%(prio)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
            else:
                link_del = ''

            txt += '<tr prio="%s" id="%s"%s><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td></tr>' % (
                prio, prio, extra, link, document_root, doms, logging, link_del)

        txt += '</table>'
        txt += '''
                      <script type="text/javascript">
                      $(document).ready(function() {
                        $("#%(name)s tr:even').addClass('alt')");

                        $('#%(name)s').tableDnD({
                          onDrop: function(table, row) {
                              var rows = table.tBodies[0].rows;
                              var post = 'update_prio=1&prios=';
                              for (var i=1; i<rows.length; i++) {
                                post += rows[i].id + ',';
                              }
                              jQuery.post ('%(url)s', post,
                                  function (data, textStatus) {
                                      window.location.reload();
                                  }
                              );
                          }
                        });
                      });

                      $(document).ready(function(){
                        $("table.rulestable tr:odd").addClass("odd");
                      });

                      $(document).mouseup(function(){
                        $("table.rulestable tr:even").removeClass("odd");
                        $("table.rulestable tr:odd").addClass("odd");
                      });
                      </script>
               ''' % {'name':   table_name,
                      'url' :   '/vserver/ajax_update'}

        # Add new Virtual Server
        table = Table(3,1)
        table += ('Nickname', 'Document Root')
        fo1 = Form ("/vserver", add_submit=False, auto=False)
        en1 = self.InstanceEntry ("new_vserver_name",  "text", size=20)
        en2 = self.InstanceEntry ("new_vserver_droot", "text", size=40)
        table += (en1, en2, SUBMIT_ADD)

        txt += "<h2>Add new Virtual Server</h2>"
        txt += fo1.Render(str(table))

        # Clone Virtual Server
        table = Table(3,1, header_style='width="250px"')
        table += ('Virtual Server', 'Clone as..')
        fo1 = Form ("/vserver", add_submit=False, auto=False)

        clonable = []
        for v in sorted_vservers:
            nick = self._cfg.get_val('vserver!%s!nick'%(v))
            clonable.append((v,nick))

        op1 = self.InstanceOptions ("vserver_clone_src", clonable)
        en1 = self.InstanceEntry   ("vserver_clone_trg", "text", size=40)
        table += (op1[0], en1, SUBMIT_CLONE)

        txt += "<h2>Clone Virtual Server</h2>"
        txt += fo1.Render(str(table))

        return txt

    def _get_vserver_for_nick (self, nick):
        for v in self._cfg['vserver']:
            n = self._cfg.get_val ('vserver!%s!nick'%(v))
            if n == nick:
                return v

    def _get_next_new_vserver (self):
        n = 1
        for v in self._cfg['vserver']:
            nv = int(v)
            if nv > n:
                n = nv
        return str(n+10)

    def _op_clone_vserver (self, post):
        # Fetch data
        prio_source = post.pop('vserver_clone_src')
        nick_target = post.pop('vserver_clone_trg')
        prio_target = self._get_next_new_vserver()

        # Check the field has been filled out
        if not nick_target:
            self._error_add ('vserver_clone_trg', '', 'Cannot be empty')
            return

        # Clone it
        error = self._cfg.clone('vserver!%s'%(prio_source), 
                                'vserver!%s'%(prio_target))
        if not error:
            self._cfg['vserver!%s!nick'%(prio_target)] = nick_target
            return '/vserver/%s'%(prio_target)

    def _op_add_vserver (self, post):
        # Ensure that no entry in empty
        for key in ['new_vserver_name', 'new_vserver_droot']:
            if not post.get_val(key):
                self._error_add (key, '', 'Cannot be empty')

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        # Find a new vserver number
        num   = self._get_next_new_vserver()
        name  = post.pop('new_vserver_name')
        droot = post.pop('new_vserver_droot')
        pre   = 'vserver!%s' % (num)

        # Do not add the server if it already exists
        if name in self._cfg['vserver']:
            return '/vserver'

        self._cfg['%s!nick'                   % (pre)] = name
        self._cfg['%s!document_root'          % (pre)] = droot
        self._cfg['%s!rule!1!match'           % (pre)] = 'default'
        self._cfg['%s!rule!1!handler'         % (pre)] = 'common'

        self._cfg['%s!rule!2!match'           % (pre)] = 'directory'
        self._cfg['%s!rule!2!match!directory' % (pre)] = '/icons'
        self._cfg['%s!rule!2!handler'         % (pre)] = 'file'
        self._cfg['%s!rule!2!document_root'   % (pre)] = CHEROKEE_ICONSDIR

        self._cfg['%s!rule!3!match'           % (pre)] = 'directory'
        self._cfg['%s!rule!3!match!directory' % (pre)] = '/cherokee_themes'
        self._cfg['%s!rule!3!handler'         % (pre)] = 'file'
        self._cfg['%s!rule!3!document_root'   % (pre)] = CHEROKEE_THEMEDIR

        return '/vserver/%s' % (num)
