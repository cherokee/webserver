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
    ("vserver!.*?!(directory|extensions|request)!.*?!priority", 
                                             validations.is_positive_int),
    ("vserver!.*?!user_dir!(directory|extensions|request)!.*?!priority", 
                                             validations.is_positive_int),
    ("add_new_priority",                     validations.is_positive_int),
    ("userdir_add_new_priority",             validations.is_positive_int)
]

LOGGER_ACCESS_NOTE = """
<p>This is the log where the information on the requests that the server receives is stored.</p>
"""

LOGGER_ERROR_NOTE = """
<p>Invalid requests and unexpected issues are logged here:</p>
"""

class PageVServer (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'vserver', cfg)
        FormHelper.__init__ (self, 'vserver', cfg)

        self._priorities         = None
        self._priorities_userdir = None

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
            if post.get_val('add_new_entry') and \
               post.get_val('add_new_priority'):
                # It's adding a new entry
                re = self._op_add_new_entry (post       = post,
                                             cfg_prefix = 'vserver!%s' %(host),
                                             url_prefix = '/vserver/%s'%(host))
                if not self.has_errors() and re:
                    return re
            elif post.get_val('userdir_add_new_entry') and \
                 post.get_val('userdir_add_new_priority'):
                # It's adding a new user entry 
                re = self._op_add_new_entry (post       = post,
                                             cfg_prefix = 'vserver!%s!user_dir'%(host),
                                             url_prefix = '/vserver/%s/userdir'%(host),
                                             key_prefix = 'userdir_')
                if not self.has_errors() and re:
                    return re
            else:
                # It's updating properties
                self._op_apply_changes (host, uri, post)

        elif uri.endswith('/ajax_update'):
            self._op_apply_changes (host, uri, post)
            return 'ok'

        self._priorities         = VServerEntries (host, self._cfg)
        self._priorities_userdir = VServerEntries (host, self._cfg, user_dir=True)

        return self._op_render_vserver_details (host)

    def _op_add_new_entry (self, post, cfg_prefix, url_prefix, key_prefix=''):
        key_add_new_type     = key_prefix + 'add_new_type'
        key_add_new_entry    = key_prefix + 'add_new_entry'
        key_add_new_handler  = key_prefix + 'add_new_handler'
        key_add_new_priority = key_prefix + 'add_new_priority'

        # The 'add_new_entry' checking function depends on 
        # the whether 'add_new_type' is a directory, an extension
        # or a regular extension
        validation = DATA_VALIDATION[:]

        type_ = post.get_val(key_add_new_type)
        if type_ == 'directory':
            validation += [(key_add_new_entry, validations.is_dir_formated)]
        elif type_ == 'extensions':
            validation += [(key_add_new_entry, validations.is_safe_id_list)]
        elif type_ == 'request':
            validation += [(key_add_new_entry, validations.is_regex)]

        # Apply changes
        self._ValidateChanges (post, validation)
        if self.has_errors():
            return

        entry    = post.pop(key_add_new_entry)
        type_    = post.pop(key_add_new_type)
        handler  = post.pop(key_add_new_handler)
        priority = post.pop(key_add_new_priority)

        pre = "%s!%s!%s" % (cfg_prefix, type_, entry)
        self._cfg["%s!handler"%(pre)]  = handler
        self._cfg["%s!priority"%(pre)] = priority

        return "%s/prio/%s" % (url_prefix, priority)

    def _op_render_vserver_details (self, host):
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
        self.AddTableEntry (table, 'Document Root',     '%s!document_root' % (pre))
        self.AddTableEntry (table, 'Directory Indexes', '%s!directory_index' % (pre))
        tabs += [('Basics', str(table))]

        # Domains
        tmp = self._render_hosts(host)
        tabs += [('Domain names', tmp)]
        
        # Behaviour
        tmp = self._render_rules_generic (cfg_key    = 'vserver!%s' %(host), 
                                          url_prefix = '/vserver/%s'%(host),
                                          priorities = self._priorities)
        tmp += self._render_add_rule(host)
        tabs += [('Behaviour', tmp)]

        # Personal Webs
        tmp  = self._render_personal_webs (host)
        if self._cfg.get_val('vserver!%s!user_dir'%(host)):
            tmp += "<p><hr /></p>"
            tmp += self._render_rules_generic (cfg_key    = 'vserver!%s!user_dir'%(host), 
                                               url_prefix = '/vserver/%s/userdir'%(host),
                                               priorities = self._priorities_userdir)
            tmp += self._render_add_rule (host, prefix="userdir_")
        tabs += [('Personal Webs', tmp)]

        # Error handlers
        tmp = self._render_error_handler(host)
        tabs += [('Error handler', tmp)]        

        # Logging
        tmp = self._render_logger(host)
        tabs += [('Logging', tmp)]

        # Security
        table = Table(2)
        self.AddTableEntry (table, 'Certificate',     '%s!ssl_certificate_file' % (pre))
        self.AddTableEntry (table, 'Certificate key', '%s!ssl_certificate_key_file' % (pre))
        self.AddTableEntry (table, 'CA List',         '%s!ssl_ca_list_file' % (pre))
        tabs += [('Security', str(table))]

        txt += self.InstanceTab (tabs)

        form = Form (self.submit_url)
        return form.Render(txt)

    def _render_error_handler (self, host):
        txt = ''
        pre = 'vserver!%s' % (host)
        
        table = Table(2)
        e = self.AddTableOptions_Reload (table, 'Error Handler',
                                         '%s!error_handler' % (pre), 
                                         ERROR_HANDLERS)
        txt += str(table) + self.Indent(e)

        return txt

    def _render_add_rule (self, host, prefix=''):
        # Add new rule
        txt      = ''
        entry    = self.InstanceEntry (prefix+'add_new_entry', 'text')
        type     = EntryOptions (prefix+'add_new_type',    ENTRY_TYPES, selected='directory')
        handler  = EntryOptions (prefix+'add_new_handler', HANDLERS,    selected='common')
        priority = self.InstanceEntry (prefix+'add_new_priority', 'text')
        
        table  = Table(4,1)
        table += ('Entry', 'Type', 'Handler', 'Priority')
        table += (entry, type, handler, priority)

        txt += "<h3>Add new rule</h3>"
        txt += str(table)
        return txt

    def _render_rules_generic (self, cfg_key, url_prefix, priorities):
        txt = ''

        if len(priorities):
            txt += '<h3>Rule list</h3>'

            table  = Table(5,1,style='width="100%%"')
            table += ('', 'Type', 'Handler', 'Priority', '')
        
            # Rule list
            for rule in priorities:
                type, name, prio, conf = rule

                pre  = '%s!%s!%s' % (cfg_key, type, name)
                link = '<a href="%s/prio/%s">%s</a>' % (url_prefix, prio, name)
                e1   = EntryOptions ('%s!handler' % (pre), HANDLERS, selected=conf['handler'].value)
                e2   = self.InstanceEntry ('%s!priority' % (pre), 'text', value=prio)

                if not (type == 'directory' and name == '/'):
                    js = "post_del_key('%s', '%s');" % (self.submit_ajax_url, pre)
                    link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                else:
                    link_del = ''

                table += (link, type, e1, e2, link_del)
            txt += str(table)
        return txt

    def _render_personal_webs (self, host):
        txt = ''

        table = Table(3)
        cfg_key = 'vserver!%s!user_dir'%(host)
        en = self.InstanceEntry (cfg_key, 'text')
        if self._cfg.get_val(cfg_key):
            js = "post_del_key('%s','%s');" % (self.submit_ajax_url, cfg_key)
            bu = self.InstanceButton ("Disable", onClick=js)
        else: 
            bu = ''
        table += ('Directory name', en, bu)
        txt += str(table)

        return txt

    def _render_logger (self, host):
        pre = 'vserver!%s!logger'%(host)

        # Logger
        table = Table(2)
        self.AddTableOptions_Ajax (table, 'Format', pre, LOGGERS)

        txt  = '<h3>Logging Format</h3>'
        txt += self.Indent(str(table))
        
        # Writers
        if self._cfg.get_val(pre):
            writers = ''

            # Accesses
            cfg_key = "%s!access!type"%(pre)
            table = Table(2)
            self.AddTableOptions_Ajax (table, 'Accesses', cfg_key, LOGGER_WRITERS)
            writers += self.Dialog(LOGGER_ACCESS_NOTE)
            writers += str(table)

            access = self._cfg.get_val(cfg_key)
            if access == 'file':
                t1 = Table(2)
                self.AddTableEntry (t1, 'Filename', '%s!access!filename' % (pre))
                writers += self.Indent(t1)
            elif access == 'exec':
                t1 = Table(2)
                self.AddTableEntry (t1, 'Command', '%s!access!command' % (pre))
                writers += self.Indent(t1)

            # Error
            cfg_key = "%s!error!type"%(pre)
            table = Table(2)
            self.AddTableOptions_Ajax (table, 'Errors', cfg_key, LOGGER_WRITERS)
            writers += self.Dialog(LOGGER_ERROR_NOTE)
            writers += str(table)

            error = self._cfg.get_val(cfg_key)
            if error == 'file':
                t1 = Table(2)
                self.AddTableEntry (t1, 'Filename', '%s!error!filename' % (pre))
                writers += self.Indent(t1)
            elif error == 'exec':
                t1 = Table(2)
                self.AddTableEntry (t1, 'Command', '%s!error!command' % (pre))
                writers += self.Indent(t1)

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
                bu = self.InstanceButton ("Del", onClick=js)
                table += (en, bu)

            txt += str(table)

        # Look for firs available
        i = 1
        while cfg_domains:
            if not cfg_domains[str(i)]:
                available = str(i)
                break
            i += 1

        # Add new domain
        txt += "<h3>Add new domain name</h3>"
        table = Table(2)
        cfg_key = "vserver!%s!domain!%s" % (host, available)
        en = self.InstanceEntry (cfg_key, 'text')
        bu = self.InstanceButton ("Add", onClick="submit();")
        table += (en, bu)

        txt += str(table)
        return txt

    def _op_apply_changes (self, host, uri, post):
        pre = "vserver!%s" % (host)

        # Error handler
        self.ApplyChanges_OptionModule ('%s!error_handler'%(pre), uri, post)

        # Apply changes
        self.ApplyChanges ([], post, DATA_VALIDATION)

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
