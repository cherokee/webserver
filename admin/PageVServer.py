import validations

from Page import *
from Form import *
from Table import *
from Entry import *
from consts import *
from RuleList import *
from CherokeeManagement import *

DATA_VALIDATION = [
    ("vserver!.*?!document_root",             (validations.is_local_dir_exists, 'cfg')),
    ("vserver!.*?!ssl_certificate_file",      (validations.is_local_file_exists, 'cfg')),
    ("vserver!.*?!ssl_certificate_key_file",  (validations.is_local_file_exists, 'cfg')),
    ("vserver!.*?!ssl_ca_list_file",          (validations.is_local_file_exists, 'cfg')),
    ("vserver!.*?!logger!.*?!filename",       (validations.parent_is_dir, 'cfg')),
    ("vserver!.*?!logger!.*?!command",        (validations.is_local_file_exists, 'cfg')),
]

RULE_LIST_NOTE = """
<p>Rules are evaluated from <b>top to bottom</b>. Drag & drop them to reorder.</p>
"""

NOTE_NICKNAME        = 'Nickname for the virtual server.'
NOTE_CERT            = 'This directive points to the PEM-encoded Certificate file for the server.'
NOTE_CERT_KEY        = 'PEM-encoded Private Key file for the server.'
NOTE_CA_LIST         = 'File with the certificates of Certification Authorities (CA) whose clients you deal with.'
NOTE_ERROR_HANDLER   = 'Allows the selection of how to generate the error responses.'
NOTE_PERSONAL_WEB    = 'Directory inside the user home directory to use as root web directory. Disabled if empty.'
NOTE_DISABLE_PW      = 'The personal web support is currently turned on.'
NOTE_ADD_DOMAIN      = 'Adds a new domain name. Wildcards are allowed in the domain name.'
NOTE_DOCUMENT_ROOT   = 'Virtual Server root directory.'
NOTE_DIRECTORY_INDEX = 'List of name files that will be used as directory index. Eg: <em>index.html,index.php</em>.'
NOTE_DISABLE_LOG     = 'The Logging is currently enabled.'
NOTE_LOGGERS         = 'Logging format. Apache compatible is highly recommended here.'
NOTE_ACCESSES        = 'Back-end used to store the log accesses.'
NOTE_ERRORS          = 'Back-end used to store the log errors.'
NOTE_ACCESSES_ERRORS = 'Back-end used to store the log accesses and errors.'
NOTE_WRT_FILE        = 'Full path to the file where the information will be saved.'
NOTE_WRT_EXEC        = 'Path to the executable that will be invoked on each log entry.'

TXT_NO  = "<i>No</i>"
TXT_YES = "<i>Yes</i>"

HELPS = [
    ('config_virtual_servers', "Virtual Servers"),
    ('modules_loggers',        "Loggers"),
    ('cookbook_ssl',           "SSL cookbook")
]


class PageVServer (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'vserver', cfg, HELPS)
        FormHelper.__init__ (self, 'vserver', cfg)

        self._priorities         = None
        self._priorities_userdir = None
        self._rule_table         = 1

    def _op_handler (self, uri, post):
        assert (len(uri) > 1)

        host = uri.split('/')[1]
        self.set_submit_url ('/vserver/%s'%(host))
        self.submit_ajax_url = "/vserver/%s/ajax_update"%(host) 

        # Check whether host exists
        cfg = self._cfg['vserver!%s'%(host)]
        if not cfg:
            return '/vserver/'
        if not cfg.has_child():
            return '/vserver/'

        default_render = False 
        if post.get_val('is_submit'):
            if post.get_val('tmp!new_rule!value'):
                re = self._op_add_new_entry (post       = post,
                                             cfg_prefix = 'vserver!%s!rule' %(host),
                                             url_prefix = '/vserver/%s'%(host),
                                             key_prefix = 'tmp!new_rule')
                if not self.has_errors() and re:
                    return re

            elif post.get_val('tmp!new_rule!user_dir!value'):
                re = self._op_add_new_entry (post       = post,
                                             cfg_prefix = 'vserver!%s!user_dir!rule' %(host),
                                             url_prefix = '/vserver/%s/userdir'%(host),
                                             key_prefix = 'tmp!new_rule!user_dir')
                if not self.has_errors() and re:
                    return re

            else:
                # It's updating properties
                re = self._op_apply_changes (host, uri, post)
                if re: return re

        elif uri.endswith('/ajax_update'):
            if post.get_val('update_prio'):
                cfg_key = post.get_val('update_prefix')
                rules = RuleList(self._cfg, cfg_key)
                
                changes = []
                for tmp in post['update_prio']:
                    changes.append(tmp.split(','))

                rules.change_prios (changes)
                return "ok"

            self.ApplyChangesDirectly (post)
            return 'ok'

        # Ensure the default rules are set
        if self._cfg.get_val('vserver!%s!user_dir'%(host)):
            tmp = self._cfg["vserver!%s!user_dir!rule"%(host)]
            if not tmp:
                pre = "vserver!%s!user_dir!rule!1" %(host)
                self._cfg["%s!match"       %(pre)] = "default"
                self._cfg["%s!handler"     %(pre)] = "common"
                self._cfg["%s!match!final" %(pre)] = "1"

        self._priorities         = RuleList(self._cfg, 'vserver!%s!rule'%(host))
        self._priorities_userdir = RuleList(self._cfg, 'vserver!%s!user_dir!rule'%(host))

        return self._op_render_vserver_details (host)

    def _op_add_new_entry (self, post, cfg_prefix, url_prefix, key_prefix):
        # Build the configuration prefix
        rules = RuleList(self._cfg, cfg_prefix)
        priority = rules.get_highest_priority() + 100
        pre = '%s!%d' % (cfg_prefix, priority)
        
        # Read the properties
        filtered_post = {}
        for p in post:
            if not p.startswith(key_prefix):
                continue

            prop = p[len('%s!'%(key_prefix)):]
            if not prop: continue

            filtered_post[prop] = post[p][0]

        # Look for the rule type
        _type = post.get_val (key_prefix)

        # The 'add_new_entry' checking function depends on 
        # the whether 'add_new_type' is a directory, an extension
        # or a regular extension
        rule_module = module_obj_factory (_type, self._cfg, pre, self.submit_url)

        # Validate
        validation = DATA_VALIDATION[:]
        validation += rule_module.validation

        self._ValidateChanges (post, validation)
        if self.has_errors():
            return
        
        # Apply the changes to the configuration tree
        self._cfg['%s!match'%(pre)] = _type
        rule_module.apply_cfg (filtered_post)

        # Get to the details page
        return "%s/rule/%d" % (url_prefix, priority)

    def _op_render_vserver_details (self, host):
        content = self._render_vserver_guts (host)
        nick = self._cfg.get_val ('vserver!%s!nick'%(host))

        self.AddMacroContent ('title', 'Virtual Server: %s' %(nick))
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _render_vserver_guts (self, host):
        pre = "vserver!%s" % (host)
        cfg = self._cfg[pre]
        name = self._cfg.get_val ('vserver!%s!nick'%(host))
        
        tabs = []
        txt = "<h1>Virtual Server: %s</h1>" % (name)

        # Basics
        table = TableProps()
        if host != "default":
            self.AddPropEntry (table, 'Virtual Server nickname', '%s!nick'%(pre), NOTE_NICKNAME)
        self.AddPropEntry (table, 'Document Root',     '%s!document_root'%(pre),   NOTE_DOCUMENT_ROOT)
        self.AddPropEntry (table, 'Directory Indexes', '%s!directory_index'%(pre), NOTE_DIRECTORY_INDEX)
        tabs += [('Basics', str(table))]

        # Domains
        tmp = self._render_hosts(host)
        tabs += [('Domain names', tmp)]
        
        # Behavior
        pre = 'vserver!%s!rule' %(host)
        tmp = self._render_rules_generic (cfg_key    = pre, 
                                          url_prefix = '/vserver/%s'%(host),
                                          priorities = self._priorities)
        tmp += self._render_add_rule ("tmp!new_rule")
        tabs += [('Behavior', tmp)]

        # Personal Webs
        tmp  = self._render_personal_webs (host)
        if self._cfg.get_val('vserver!%s!user_dir'%(host)):
            tmp += "<p><hr /></p>"
            pre = 'vserver!%s!user_dir!rule'%(host)
            tmp += self._render_rules_generic (cfg_key    = pre, 
                                               url_prefix = '/vserver/%s/userdir'%(host),
                                               priorities = self._priorities_userdir)
            tmp += self._render_add_rule ("tmp!new_rule!user_dir")
        tabs += [('Personal Webs', tmp)]

        # Error handlers
        tmp = self._render_error_handler(host)
        tabs += [('Error handler', tmp)]        

        # Logging
        tmp = self._render_logger(host)
        tabs += [('Logging', tmp)]

        # Security
        pre = 'vserver!%s' % (host)

        table = TableProps()
        self.AddPropEntry (table, 'Certificate',     '%s!ssl_certificate_file' % (pre),     NOTE_CERT)
        self.AddPropEntry (table, 'Certificate key', '%s!ssl_certificate_key_file' % (pre), NOTE_CERT_KEY)
        self.AddPropEntry (table, 'CA List',         '%s!ssl_ca_list_file' % (pre),         NOTE_CA_LIST)
        tabs += [('Security', str(table))]

        txt += self.InstanceTab (tabs)

        form = Form (self.submit_url, add_submit=False)
        return form.Render(txt)

    def _render_error_handler (self, host):
        txt = ''
        pre = 'vserver!%s' % (host)
        
        table = TableProps()
        e = self.AddPropOptions_Reload (table, 'Error Handler',
                                        '%s!error_handler' % (pre), 
                                        modules_available(ERROR_HANDLERS), 
                                        NOTE_ERROR_HANDLER)
        txt += str(table) + self.Indent(e)

        return txt
    
    def _render_add_rule (self, prefix):
        # Render
        txt = "<h2>Add new rule</h2>"
        table = TableProps()
        e = self.AddPropOptions_Reload (table, "Rule Type", prefix, 
                                        modules_available(RULES), "")
        txt += self.Indent (str(table) + e)
        return txt

    def _get_handler_name (self, mod_name):
        for h in HANDLERS:
            if h[0] == mod_name:
                return h[1]

    def _get_auth_name (self, mod_name):
        for h in VALIDATORS:
            if h[0] == mod_name:
                return h[1]

    def _render_rules_generic (self, cfg_key, url_prefix, priorities):
        txt = ''

        if not len(priorities):
            return txt

        txt += self.Dialog(RULE_LIST_NOTE)

        table_name = "rules%d" % (self._rule_table)
        self._rule_table += 1

        txt += '<table id="%s" class="rulestable">' % (table_name)
        txt += '<tr NoDrag="1" NoDrop="1"><th>Target</th><th>Type</th><th>Handler</th><th>Auth</th><th>Enc</th><th>Exp</th><th>Final</th></tr>'

        # Rule list
        for prio in priorities:
            conf = priorities[prio]

            _type = conf.get_val('match')
            pre   = '%s!%s' % (cfg_key, prio)

            # Try to load the rule plugin
            rule_module = module_obj_factory (_type, self._cfg, pre, self.submit_url)
            name        = rule_module.get_name()
            name_type   = rule_module.get_type_name()

            if _type != 'default':
                link     = '<a href="%s/rule/%s">%s</a>' % (url_prefix, prio, name)
                js       = "post_del_key('%s', '%s');" % (self.submit_ajax_url, pre)
                final    = self.InstanceCheckbox ('%s!match!final'%(pre), True, quiet=True)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                extra    = ''
            else:
                link     = '<a href="%s/rule/%s">Default</a>' % (url_prefix, prio)
                extra    = ' NoDrag="1" NoDrop="1"'
                final    = self.HiddenInput ('%s!match!final'%(pre), "1")
                link_del = ''

            if conf.get_val('handler'):
                handler_name = self._get_handler_name (conf['handler'].value)
            else:
                handler_name = TXT_NO

            if conf.get_val('auth'):
                auth_name = self._get_auth_name (conf['auth'].value)
            else:
                auth_name = TXT_NO

            expiration = [TXT_NO, TXT_YES]['expiration' in conf.keys()]

            encoders = TXT_NO
            if 'encoder' in conf.keys():
                for k in conf['encoder'].keys():
                    if int(conf.get_val('encoder!%s'%(k))):
                        encoders = TXT_YES

            txt += '<!-- %s --><tr prio="%s" id="%s"%s><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n' % (
                prio, pre, prio, extra, link, name_type, handler_name, auth_name, encoders, expiration, final, link_del)

        txt += '</table>\n'
        txt += '''
                      <script type="text/javascript">
                      $(document).ready(function() {
                        $("#%(name)s tr:even').addClass('alt')");

                        $('#%(name)s').tableDnD({
                          onDrop: function(table, row) {
                              var rows = table.tBodies[0].rows;
                              var post = 'update_prefix=%(prefix)s&';
                              for (var i=1; i<rows.length; i++) {
                                var prio = (rows.length - i) * 100;
                                post += 'update_prio=' + rows[i].id + ',' + prio + '&';
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
                      'url' :   self.submit_ajax_url,
                      'prefix': cfg_key}
        return txt

    def _render_personal_webs (self, host):
        txt = ''

        table = TableProps()
        cfg_key = 'vserver!%s!user_dir'%(host)
        if self._cfg.get_val(cfg_key):
            js = "post_del_key('%s','%s');" % (self.submit_ajax_url, cfg_key)
            button = self.InstanceButton ("Disable", onClick=js)
            self.AddProp (table, 'Status', '', button, NOTE_DISABLE_PW)

        self.AddPropEntry (table, 'Directory name', cfg_key, NOTE_PERSONAL_WEB)
        txt += str(table)

        return txt

    def _render_logger (self, host):
        txt   = ""

        # Disable
        pre = 'vserver!%s!logger'%(host)
        cfg = self._cfg[pre]
        format = self._cfg.get_val(pre)
        if cfg and cfg.has_child():
            table = TableProps()
            js = "post_del_key('%s','%s');" % (self.submit_ajax_url, pre)
            button = self.InstanceButton ("Disable", onClick=js)
            self.AddProp (table, 'Status', '', button, NOTE_DISABLE_LOG)
            txt += str(table)
        txt += "<hr />"

        # Logger
        txt += '<h3>Logging Format</h3>'
        table = TableProps()
        self.AddPropOptions_Ajax (table, 'Format', pre, 
                                  modules_available(LOGGERS), NOTE_LOGGERS)
        txt += self.Indent(str(table))

        # Writers

        if format:
            writers = ''

            # Accesses & Error together
            if format == 'w3c':
                cfg_key = "%s!all!type"%(pre)
                table = TableProps()
                self.AddPropOptions_Ajax (table, 'Accesses and Errors', cfg_key, 
                                          LOGGER_WRITERS, NOTE_ACCESSES_ERRORS)
                writers += str(table)

                all = self._cfg.get_val(cfg_key)
                if not all or all == 'file':
                    t1 = TableProps()
                    self.AddPropEntry (t1, 'Filename', '%s!all!filename'%(pre), NOTE_WRT_FILE)
                    writers += str(t1)
                elif all == 'exec':
                    t1 = TableProps()
                    self.AddPropEntry (t1, 'Command', '%s!all!command'%(pre), NOTE_WRT_EXEC)
                    writers += str(t1)

            else:
                # Accesses
                cfg_key = "%s!access!type"%(pre)
                table = TableProps()
                self.AddPropOptions_Ajax (table, 'Accesses', cfg_key, LOGGER_WRITERS, NOTE_ACCESSES)
                writers += str(table)

                access = self._cfg.get_val(cfg_key)
                if not access or access == 'file':
                    t1 = TableProps()
                    self.AddPropEntry (t1, 'Filename', '%s!access!filename'%(pre), NOTE_WRT_FILE)
                    writers += str(t1)
                elif access == 'exec':
                    t1 = TableProps()
                    self.AddPropEntry (t1, 'Command', '%s!access!command'%(pre), NOTE_WRT_EXEC)
                    writers += str(t1)

                writers += "<hr />"

                # Error
                cfg_key = "%s!error!type"%(pre)
                table = TableProps()
                self.AddPropOptions_Ajax (table, 'Errors', cfg_key, LOGGER_WRITERS, NOTE_ERRORS)
                writers += str(table)

                error = self._cfg.get_val(cfg_key)
                if not error or error == 'file':
                    t1 = TableProps()
                    self.AddPropEntry (t1, 'Filename', '%s!error!filename'%(pre), NOTE_WRT_FILE)
                    writers += str(t1)
                elif error == 'exec':
                    t1 = TableProps()
                    self.AddPropEntry (t1, 'Command', '%s!error!command'%(pre), NOTE_WRT_EXEC)
                    writers += str(t1)

            txt += '<h3>Writers</h3>'
            txt += self.Indent(writers)

        return txt

    def _render_hosts (self, host):
        cfg_domains = self._cfg["vserver!%s!domain"%(host)]

        txt       = ""
        available = "1"

        if cfg_domains and \
           cfg_domains.has_child():
            table = Table(2,1)
            table += ('Domain pattern', '')

            # Build list
            for i in cfg_domains:
                domain = cfg_domains[i].value
                cfg_key = "vserver!%s!domain!%s" % (host, i)
                en = self.InstanceEntry (cfg_key, 'text')
                js = "post_del_key('%s','%s');" % (self.submit_ajax_url, cfg_key)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (en, link_del)

            txt += str(table)
            txt += "<hr />"

        # Look for firs available
        i = 1
        while cfg_domains:
            if not cfg_domains[str(i)]:
                available = str(i)
                break
            i += 1

        # Add new domain
        table = TableProps()
        cfg_key = "vserver!%s!domain!%s" % (host, available)
        self.AddPropEntry (table, 'Add new domain name', cfg_key, NOTE_ADD_DOMAIN)
        txt += str(table)

        return txt

    def _op_apply_changes (self, host, uri, post):
        pre = "vserver!%s" % (host)

        # Error handler
        self.ApplyChanges_OptionModule ('%s!error_handler'%(pre), uri, post)

        # Look for the checkboxes
        checkboxes = []
        tmp = self._cfg['%s!rule'%(pre)]
        if tmp and tmp.has_child():
            for p in tmp:
                checkboxes.append('%s!rule!%s!match!final'%(pre,p))

        # Apply changes
        self.ApplyChanges (checkboxes, post, DATA_VALIDATION)

        # Clean old logger properties
        self._cleanup_logger_cfg (host)

    def _cleanup_logger_cfg (self, host):
        cfg_key = "vserver!%s!logger" % (host)
        logger = self._cfg.get_val (cfg_key)
        if not logger:
            del(self._cfg[cfg_key])
            return

        to_be_deleted = []
        for entry in self._cfg[cfg_key]:
            if logger == "stderr" or \
               logger == "syslog":
                to_be_deleted.append(cfg_key)
            elif logger == "file" and \
                 entry != "filename":
                to_be_deleted.append(cfg_key)
            elif logger == "exec" and \
                 entry != "command":
                to_be_deleted.append(cfg_key)

        for entry in to_be_deleted:
            key = "%s!%s" % (cfg_key, entry)
            del(self._cfg[key])
