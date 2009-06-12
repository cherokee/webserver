import validations

from Page import *
from Form import *
from Table import *
from Entry import *
from Wizard import *

# For gettext
N_ = lambda x: x

DATA_VALIDATION = [
    ("new_vserver_name",   validations.is_safe_id),
    ("new_vserver_droot", (validations.is_dev_null_or_local_dir_exists, 'cfg')),
    ("vserver_clone_trg",  validations.is_safe_id),
]

COMMENT = N_("""
<p>'Virtual Server' is an abstraction mechanism that allows to define
a custom number of parameters and rules that have to be applied to one or
more domains.</p>
""")

HELPS = [
    ('config_virtual_servers', N_("Virtual Servers"))
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

        self.AddMacroContent ('title', _('Virtual Servers configuration'))
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _op_handler (self, uri, post):
        if '/wizard/' in uri:
            re = self._op_apply_wizard (uri, post)
            if re: return re

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

    def _op_apply_wizard (self, uri, post):
        tmp  = uri.split('/')
        name = tmp[2]

        mgr = WizardManager (self._cfg, "VServer", 'vserver')
        wizard = mgr.load (name)

        output = wizard.run ("/vserver%s"%(uri), post)
        if output:
            return output

        return '/vserver'

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
        txt = "<h1>%s</h1>" % (_('Virtual Servers'))
        txt += self.Dialog (COMMENT)

        vservers = self._cfg['vserver']
        table_name = 'vserver_sortable_table'

        def sort_vservers(x,y):
            return cmp(int(x), int(y))

        sorted_vservers = self._cfg['vserver'].keys()
        sorted_vservers.sort(sort_vservers, reverse=True)

        txt += '<div class="rulesdiv"><table id="%s" class="rulestable">' % (table_name)
        txt += '<tr><th>&nbsp</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th></th></tr>' % \
            (_('Nickname'), _('Root'), _('Domains'), _('Logging'))

        ENABLED_IMAGE  = self.InstanceImage('tick.png', _('Yes'))
        DISABLED_IMAGE = self.InstanceImage('cross.png', _('No'))

        for prio in sorted_vservers:
            nick          = self._cfg.get_val('vserver!%s!nick'%(prio))
            document_root = self._cfg.get_val('vserver!%s!document_root'%(prio), '')
            logger_val    = self._cfg.get_val('vserver!%s!logger'%(prio))

            hmatchtype    = self._cfg.get_val('vserver!%s!match'%(prio), None)
            if hmatchtype == 'rehost':
                skey = 'regex'
            elif hmatchtype == 'wildcard':
                skey = 'domain'
            else:
                doms = 1

            if hmatchtype:
                prefix        = 'vserver!%s!match!%s!%%s' % (prio,skey)
                domkeys       = [prefix % k for k in self._cfg.keys(prefix[:-3])]
                domains       = self._cfg.get_vals(domkeys)
                if domains:
                    doms = len(domains)
                    if not nick in domains:
                        doms += 1
                else:
                    doms = 1

            link = '<a href="/vserver/%s">%s</a>' % (prio, nick)
            if nick == 'default':
                extra = ' class="nodrag nodrop"'
                draggable = ''
            else:
                extra = ''
                draggable = ' class="dragHandle"'

            if logger_val:
                logging = ENABLED_IMAGE
            else:
                logging = DISABLED_IMAGE

            if nick != "default":
                link_del = self.AddDeleteLink ('/ajax/update', 'vserver!%s'%(prio))
            else:
                link_del = ''

            txt += '<tr prio="%s" id="%s"%s><td%s>&nbsp</td><td>%s</td><td>%s</td><td class="center">%d</td><td class="center">%s</td><td class="center">%s</td></tr>' % (
                prio, prio, extra, draggable, link, document_root, doms, logging, link_del)

        txt += '</table>'


        txt += '''
                      <script type="text/javascript">
                      var prevSection = '';

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
                          },
                          dragHandle: "dragHandle"
                        });

                        $("#%(name)s tr:not(.nodrag, nodrop)").hover(function() {
                            $(this.cells[0]).addClass('dragHandleH');
                        }, function() {
                            $(this.cells[0]).removeClass('dragHandleH');
                        });

                      });

                      $(document).ready(function(){
                        $("table.rulestable tr:odd").addClass("odd");
                        $("#newsection_b").click(function() { openSection('newsection')});
                        $("#clonesection_b").click(function() { openSection('clonesection')});
                        $("#wizardsection_b").click(function() { openSection('wizardsection')});
                        open_vssec  = get_cookie('open_vssec');
                        if (open_vssec && document.referrer == window.location) {
                            openSection(open_vssec);
                        }
                      });

                      function openSection(section) 
                      {
                          document.cookie = "open_vssec="  + section;
                          if (prevSection != '') {
                              $("#"+prevSection).hide();
                              $("#"+prevSection+"_b").attr("style", "font-weight: normal;");
                          }
		          $("#"+section+"_b").attr("style", "font-weight: bold;");
                          $("#"+section).show();
                          prevSection = section;
                      }

                      $(document).mouseup(function(){
                        $("table.rulestable tr:even").removeClass("odd");
                        $("table.rulestable tr:odd").addClass("odd");
                      });
                      </script>
               ''' % {'name':   table_name,
                      'url' :   '/vserver/ajax_update'}

        # Add new Virtual Server
        txt += '<div class="rulessection" id="newsection">'
        table = Table(3, 1, header_style='width="200px"')
        table += (_('Nickname'), _('Document Root'))
        fo1 = Form ("/vserver", add_submit=False, auto=False)
        en1 = self.InstanceEntry ("new_vserver_name",  "text", size=20)
        en2 = self.InstanceEntry ("new_vserver_droot", "text", size=40)
        table += (en1, en2, SUBMIT_ADD)

        txt += "<h2>%s</h2>" % (_('Add new Virtual Server'))
        txt += self.Indent(fo1.Render(str(table)))
        txt += '</div>'

        # Clone Virtual Server
        txt += '<div class="rulessection" id="clonesection">'
        table = Table(3, 1, header_style='width="200px"')
        table += (_('Virtual Server'), _('Clone as..'))
        fo1 = Form ("/vserver", add_submit=False, auto=False)

        clonable = []
        for v in sorted_vservers:
            nick = self._cfg.get_val('vserver!%s!nick'%(v))
            clonable.append((v,nick))

        op1 = self.InstanceOptions ("vserver_clone_src", clonable)
        en1 = self.InstanceEntry   ("vserver_clone_trg", "text", size=40)
        table += (op1[0], en1, SUBMIT_CLONE)

        txt += "<h2>%s</h2>" % (_('Clone Virtual Server'))
        txt += self.Indent(fo1.Render(str(table)))
        txt += '</div>'

        # Wizards
        txt += '<div class="rulessection" id="wizardsection">'
        txt += self._render_wizards()
        txt += '</div>'

        txt += '<div class="rulesbutton"><a id="newsection_b">%s</a></div>' % (_('Add new Virtual Server'))
        txt += '<div class="rulesbutton"><a id="clonesection_b">%s</a></div>' % (_('Clone Virtual Server'))
        txt += '<div class="rulesbutton"><a id="wizardsection_b">%s</a></div>' % (_('Wizards'))
        txt += '</div>'
        
        return txt

    def _render_wizards (self):
        txt = ''
        mgr = WizardManager (self._cfg, "VServer", pre='vserver')
        txt += mgr.render ("/vserver")

        if txt: 
            txt = _("<h2>Wizards</h2>") + txt

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
        # Validate entries
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        # Fetch data
        prio_source = post.pop('vserver_clone_src')
        nick_target = post.pop('vserver_clone_trg')
        prio_target = self._get_next_new_vserver()

        # Check the field has been filled out
        if not nick_target:
            self._error_add ('vserver_clone_trg', '', _('Cannot be empty'))
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
                self._error_add (key, '', _('Cannot be empty'))

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
