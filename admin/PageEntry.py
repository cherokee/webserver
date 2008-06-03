import validations

from Page import *
from Form import *
from Table import *
from Entry import *
from RuleList import *
from Module import *
from consts import *

DEFAULT_RULE_WARNING = 'The default match ought not to be changed.'

NOTE_DOCUMENT_ROOT = 'Allow to specify an alternative document root path.'
NOTE_HANDLER       = 'How the connection will be handler.'
NOTE_HTTPS_ONLY    = 'Enable to allow access to the resource only by https.'
NOTE_ALLOW_FROM    = 'List of IPs and subnets allowed to access the resource.'
NOTE_VALIDATOR     = 'Which, if any, will be the authentication method.'

DATA_VALIDATION = [
    ("vserver!.*?!rule!(\d+)!document_root", (validations.is_local_dir_exists, 'cfg')),
    ("vserver!.*?!rule!(\d+)!allow_from",     validations.is_ip_or_netmask_list)
]

class PageEntry (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'entry', cfg)
        FormHelper.__init__ (self, 'entry', cfg)
        self._priorities = None
        self._is_userdir = False

    def _op_render (self):
        raise "no"

    def _parse_uri (self, uri):
        assert (len(uri) > 1)
        assert ("prio" in uri)
        
        # Parse the URL
        temp = uri.split('/')
        self._is_userdir = (temp[2] == 'userdir')

        if self._is_userdir:
            self._host        = temp[1]
            self._prio        = temp[4]        
            self._priorities  = RuleList (self._cfg, 'vserver!%s!user_dir!rule'%(self._host))
            self._entry       = self._priorities[int(self._prio)]
            url = '/vserver/%s/userdir/prio/%s' % (self._host, self._prio)
        else:
            self._host        = temp[1]
            self._prio        = temp[3]        
            self._priorities  = RuleList (self._cfg, 'vserver!%s!rule'%(self._host))
            self._entry       = self._priorities[int(self._prio)]
            url = '/vserver/%s/prio/%s' % (self._host, self._prio)

        # Set the submit URL
        self.set_submit_url (url)

    def _op_handler (self, uri, post):
        # Parse the URI
        self._parse_uri (uri)
        
        # Entry not found
        if not self._entry:
            return "/vserver/%s" % (self._host)

        if not self._is_userdir:
            self._conf_prefix = 'vserver!%s!rule!%s' % (self._host, self._prio)
        else:
            self._conf_prefix = 'vserver!%s!user_dir!rule!%s' % (self._host, self._prio)

        # Check what to do..
        if post.get_val('is_submit'):
            self._op_apply_changes (uri, post)

        return self._op_default (uri)
    
    def _op_apply_changes (self, uri, post):
        # Handler properties
        pre = "%s!handler" % (self._conf_prefix)
        self.ApplyChanges_OptionModule (pre, uri, post)

        # Validator properties
        pre = "%s!auth" % (self._conf_prefix)
        self.ApplyChanges_OptionModule (pre, uri, post)

        # Apply changes
        self.ApplyChanges (["%s!only_secure"%(self._conf_prefix)], post, DATA_VALIDATION)

    def _op_default (self, uri):
        # Render page
        title   = self._get_title()
        content = self._render_guts()

        self.AddMacroContent ('title',   title)
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _get_title (self, html=False):
        if html:
            txt = '<a href="/vserver/%s">%s</a> - ' % (self._host, self._host)
        else:
            txt = '%s - ' % (self._host)

        # Load the rule plugin
        _type = self._entry.get_val('match')
        rule_module = module_obj_factory (_type, self._cfg, self._conf_prefix, self.submit_url)

        txt += "%s: %s" % (rule_module.get_type_name(), rule_module.get_name())
        return txt

    def _render_guts (self):
        pre  = self._conf_prefix
        tabs = []
        
        # Rule Properties
        tabs += [('Rule', self._render_rule())]        

        # Handler
        table = TableProps()
        e = self.AddPropOptions_Reload (table, 'Handler', '%s!handler'%(pre), 
                                        HANDLERS, NOTE_HANDLER)
        self.AddPropEntry (table, 'Document Root', '%s!document_root'%(pre), NOTE_DOCUMENT_ROOT)

        if e:
            tabs += [('Handler', str(table) + e)]
        else:
            tabs += [('Handler', str(table))]

        # Security
        tabs += [('Security', self._render_security())]

        txt  = '<h1>%s</h1>' % (self._get_title (html=True))
        txt += self.InstanceTab (tabs)
        form = Form (self.submit_url)
        return form.Render(txt)

    def _render_handler_properties (self):
        pre  = "%s!handler" % (self._conf_prefix)
        name = self._cfg.get_val(pre)

        try:
            props = module_obj_factory (name, self._cfg, pre, self.submit_url)
        except IOError:
            return "Couldn't load the properties module"
        return props._op_render()

    def _render_rule (self):
        pre = "%s!match"%(self._conf_prefix)

        if self._cfg.get_val(pre) == 'default':
            return self.Dialog (DEFAULT_RULE_WARNING, 'important-information')

        # Change the rule type
        table = TableProps()
        e = self.AddPropOptions_Reload (table, "Rule Type", pre, RULES, "")
        return str(table) + e

    def _render_security (self):
        pre = self._conf_prefix

        txt   = "<h2>Access Restrictions</h2>"
        table = TableProps()
        self.AddPropCheck (table, 'Only https', '%s!only_secure'%(pre), False, NOTE_HTTPS_ONLY)
        self.AddPropEntry (table, 'Allow From',  '%s!allow_from' %(pre), NOTE_ALLOW_FROM)
        txt += self.Indent(table)

        txt += "<h2>Authentication</h2>"
        table = TableProps()
        e = self.AddPropOptions_Reload (table, 'Validation Mechanism', '%s!auth'%(pre), 
                                        VALIDATORS, NOTE_VALIDATOR)
        txt += self.Indent (table)
        txt += e
        return txt

