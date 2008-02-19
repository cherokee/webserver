import validations

from Page import *
from Form import *
from Table import *
from Entry import *
from consts import *
from VirtualServer import *

DATA_VALIDATION = [
    ("vserver!.*?!document_root",            validations.is_local_dir_exists),
    ("vserver!.*?!ssl_certificate_file",     validations.is_local_file_exists),
    ("vserver!.*?!ssl_certificate_key_file", validations.is_local_file_exists),
    ("vserver!.*?!ssl_ca_list_file",         validations.is_local_file_exists),
    ("vserver!.*?!logger!access!filename",   validations.parent_is_dir),
    ("vserver!.*?!logger!error!filename",    validations.parent_is_dir),
    ("vserver!.*?!logger!access!command",    validations.is_local_file_exists),
    ("vserver!.*?!logger!error!command",     validations.is_local_file_exists),
    ("vserver!.*?!(directory|extensions|request)!.*?!priority", validations.is_positive_int),
    ("add_new_priority",                     validations.is_positive_int)
]

class PageVServer (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'vserver', cfg)
        FormHelper.__init__ (self, 'vserver', cfg)
        self._priorities = None

    def _op_handler (self, uri, post):
        assert (len(uri) > 1)

        host = uri.split('/')[1]
        default_render = False

        if uri.endswith('/update'):
            # It's adding a new entry
            if 'add_new_entry' in post and \
                len (post['add_new_entry'][0]) > 0:
                self._op_add_new_entry (host, post)
                if self.has_errors():
                    self._priorities = VServerEntries (host, self._cfg)
                    return self._op_render_vserver_details (host, uri[len(host)+1:])
            
            # It's updating properties
            self._op_apply_changes (host, post)
            if not self.has_errors():
                return "/%s/%s" % (self._id, host)
            default_render = True

        elif uri.endswith('/ajax_update'):
            self._op_apply_changes (host, post)
            return 'ok'

        else:
            default_render = True

        if default_render:
            self._priorities = VServerEntries (host, self._cfg)
            return self._op_render_vserver_details (host, uri[len(host)+1:])

    def _op_add_new_entry (self, host, post):
        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        entry    = post['add_new_entry'][0]
        type     = post['add_new_type'][0]
        handler  = post['add_new_handler'][0]
        priority = post['add_new_priority'][0]

        pre = "vserver!%s!%s!%s" % (host, type, entry)
        self._cfg["%s!handler"%(pre)]  = handler
        self._cfg["%s!priority"%(pre)] = priority

        return "/%s/%s/prio/%s" % (self._id, host, priority)

    def _op_render_vserver_details (self, host, uri):
        content = self._render_vserver_guts (host)

        self.AddMacroContent ('title', 'Virtual Server: %s' %(host))
        self.AddMacroContent ('content', content)

        return Page.Render(self)

    def _render_vserver_guts (self, host):
        pre = "vserver!%s" % (host)
        cfg = self._cfg[pre]
        
        tabs = []
        txt = "<h1>Virtual Server: %s</h1>" % (host)

        # Basics
        table = Table(2)
        self.AddTableEntry (table, 'Document Root',   '%s!document_root' % (pre))
        self.AddTableEntry (table, 'Directory Index', '%s!directory_index' % (pre))
        tabs += [('Basics', str(table))]

        # Domains
        tmp = self._render_hosts(host)
        tabs += [('Domain names', tmp)]
        
        # Behaviour
        tmp  = self._render_rules (host, cfg['directory'], cfg['extensions'], cfg['request'])
        tmp += self._render_add_rule(host)
        tabs += [('Behaviour', tmp)]

        # Logging
        tmp   = ''
        table = Table(2)
        props = {}

        t1 = Table(2)
        self.AddTableEntry (t1, 'Filename', '%s!logger!access!filename' % (pre))
        t2 = Table(2)
        self.AddTableEntry (t2, 'Command', '%s!logger!access!command' % (pre))
        props['stderr'] = ''
        props['syslog'] = ''
        props['file']   = str(t1)
        props['exec']   = str(t2)
        e = self.AddTableOptions_w_Properties (table, "Access", '%s!logger!access!type' % (pre), 
                                               LOGGER_WRITERS, props, 'log1')
        tmp += str(table) + e

        table = Table(2)
        props = {}
        t1 = Table(2)
        self.AddTableEntry (t1, 'Filename', '%s!logger!error!filename' % (pre))
        t2 = Table(2)
        self.AddTableEntry (t2, 'Command', '%s!logger!error!command' % (pre))
        props['stderr'] = ''
        props['syslog'] = ''
        props['file']   = str(t1)
        props['exec']   = str(t2)
        e = self.AddTableOptions_w_Properties (table, "Error", '%s!logger!error!type' % (pre), 
                                               LOGGER_WRITERS, props, 'log2')
        tmp += str(table) + e        
        tabs += [('Logging', tmp)]

        # Security
        table = Table(2)
        self.AddTableEntry (table, 'Certificate',     '%s!ssl_certificate_file' % (pre))
        self.AddTableEntry (table, 'Certificate key', '%s!ssl_certificate_key_file' % (pre))
        self.AddTableEntry (table, 'CA List',         '%s!ssl_ca_list_file' % (pre))
        tabs += [('Security', str(table))]

        txt += self.InstanceTab (tabs)

        form = Form ("/%s/%s/update" % (self._id, host))
        return form.Render(txt)

    def _render_add_rule (self, host):
        # Add new rule
        txt      = ''
        entry    = self.InstanceEntry ('add_new_entry', 'text')
        type     = EntryOptions ('add_new_type',      ENTRY_TYPES, selected='directory')
        handler  = EntryOptions ('add_new_handler',   HANDLERS,    selected='common')
        priority = self.InstanceEntry ('add_new_priority', 'text')
        
        table  = Table(4,1)
        table += ('Entry', 'Type', 'Handler', 'Priority')
        table += (entry, type, handler, priority)

        txt += "<h3>Add new rule</h3>"
        txt += str(table)
        return txt

    def _render_rules (self, host, dirs_cfg, exts_cfg, reqs_cfg):
        txt    = ''
        table  = Table(4,1)
        table += ('', 'Type', 'Handler', 'Priority')
        
        # Rule list
        for rule in self._priorities:
            type, name, prio, conf = rule

            pre  = 'vserver!%s!%s!%s' % (host, type, name)
            link = '<a href="/vserver/%s/prio/%s">%s</a>' % (host, prio, name)
            e1   = EntryOptions ('%s!handler' % (pre), HANDLERS, selected=conf['handler'].value)
            e2   = self.InstanceEntry ('%s!priority' % (pre), 'text', value=prio)
            table += (link, type, e1, e2)
        txt += str(table)
        return txt

    def _render_hosts (self, host):
        cfg_domains = self._cfg["vserver!%s!domain"%(host)]

        txt       = ""
        available = "1"

        if cfg_domains:
            table = Table(2,1)
            table += ('Domain pattern', '')

            # Build list
            for i in cfg_domains:
                domain = cfg_domains[i].value
                cfg_key = "vserver!%s!domain!%s" % (host, i)
                en = self.InstanceEntry (cfg_key, 'text')
                js = "post_del_key('/vserver/%s/update','%s');" % (host, cfg_key)
                bu = self.InstanceButton ("Del", onClick=js)
                table += (en, bu)

            txt += str(table)

            # Look for firs available
            i = 1
            while True:
                if not cfg_domains[str(i)]:
                    available = str(i)
                    break
                i += 1

        DOMAIN_ADD_JS = """
        <script type="text/javascript">
        submit_new_domain = function(host, domain_id) {
            return post_add_entry_key (
                          "/vserver/"+host+"/update",
                          "new_domain",
                          "vserver!" + host + "!domain!" + domain_id);
        }
        </script>
        """

        txt += "<h3>Add new domain name</h3>"
        txt += DOMAIN_ADD_JS

        table = Table(2)
        en = self.InstanceEntry ("new_domain", 'text')
        bu = self.InstanceButton ("Add", onClick="submit_new_domain('%s','%s');"%(host, available))
        table += (en, bu)

        txt += str(table)
        return txt

    def _op_apply_changes (self, host, post):
        # Apply changes
        self.ApplyChanges ([], post, DATA_VALIDATION)

        # Clean old logger properties
        self._cleanup_logger_cfg (host)

    def _cleanup_logger_cfg (self, host):
        cfg_key = "vserver!%s!logger" % (host)
        try:
            logger  = self._cfg[cfg_key].value
        except:
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
            del(self._cfg[entry])
