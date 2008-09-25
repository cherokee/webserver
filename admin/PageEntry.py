import validations

from Page import *
from Form import *
from Table import *
from Entry import *
from RuleList import *
from Module import *
from consts import *

DEFAULT_RULE_WARNING = 'The default match ought not to be changed.'

NOTE_DOCUMENT_ROOT   = 'Allows to specify an alternative document root path.'
NOTE_HANDLER         = 'How the connection will be handled.'
NOTE_HTTPS_ONLY      = 'Enable to allow access to the resource only by https.'
NOTE_ALLOW_FROM      = 'List of IPs and subnets allowed to access the resource.'
NOTE_VALIDATOR       = 'Which, if any, will be the authentication method.'
NOTE_EXPIRATION      = 'Points how long the files should be cached'
NOTE_EXPIRATION_TIME = "How long from the object can be cached.<br />" + \
                       "The <b>m</b>, <b>h</b>, <b>d</b> and <b>w</b> suffixes are allowed for minutes, hours, days, and weeks. Eg: 2d."

DATA_VALIDATION = [
    ("vserver!.*?!rule!(\d+)!document_root", (validations.is_local_dir_exists, 'cfg')),
    ("vserver!.*?!rule!(\d+)!allow_from",     validations.is_ip_or_netmask_list)
]

HELPS = [
    ('config_virtual_servers_rule', "Behavior rules")
]

class PageEntry (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'entry', cfg, HELPS)
        FormHelper.__init__ (self, 'entry', cfg)
        self._priorities = None
        self._is_userdir = False

    def _op_render (self):
        raise "no"

    def _parse_uri (self, uri):
        assert (len(uri) > 1)
        assert ("rule" in uri)

        # Parse the URL
        temp = uri.split('/')
        self._is_userdir = (temp[2] == 'userdir')

        if self._is_userdir:
            self._host        = temp[1]
            self._prio        = temp[4]
            self._priorities  = RuleList (self._cfg, 'vserver!%s!user_dir!rule'%(self._host))
            self._entry       = self._priorities[int(self._prio)]
            url = '/vserver/%s/userdir/rule/%s' % (self._host, self._prio)
        else:
            self._host        = temp[1]
            self._prio        = temp[3]
            self._priorities  = RuleList (self._cfg, 'vserver!%s!rule'%(self._host))
            self._entry       = self._priorities[int(self._prio)]
            url = '/vserver/%s/rule/%s' % (self._host, self._prio)

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

        # Check boxes
        checks = ["%s!only_secure"%(self._conf_prefix)]
        for e,e_name in modules_available(ENCODERS):
            checks.append ('%s!encoder!%s' % (self._conf_prefix, e))

        # Apply changes
        self.ApplyChanges (checks, post, DATA_VALIDATION)

    def _op_default (self, uri):
        # Render page
        title   = self._get_title()
        content = self._render_guts()

        self.AddMacroContent ('title',   title)
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _get_title (self, html=False):
        nick = self._cfg.get_val ('vserver!%s!nick'%(self._host))

        if html:
            txt = '<a href="/vserver/%s">%s</a> - ' % (self._host, nick)
        else:
            txt = '%s - ' % (nick)

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
                                        modules_available(HANDLERS), NOTE_HANDLER)

        props = self._get_handler_properties()
        if props and props.show_document_root:
            self.AddPropEntry (table, 'Document Root', '%s!document_root'%(pre), NOTE_DOCUMENT_ROOT)

        if e:
            tabs += [('Handler', str(table) + e)]
        else:
            tabs += [('Handler', str(table))]

        self.AddHelps (module_get_help (self._cfg.get_val('%s!handler'%(pre))))

        # Encoding
        tabs += [('Encoding', self._render_encoding())]

        # Expiration
        tabs += [('Expiration', self._render_expiration())]

        # Security
        tabs += [('Security', self._render_security())]

        txt  = '<h1>%s</h1>' % (self._get_title (html=True))
        txt += self.InstanceTab (tabs)
        form = Form (self.submit_url, add_submit=False) ##add_submit=True,auto=False
        return form.Render(txt)

    def _get_handler_properties (self):
        pre  = "%s!handler" % (self._conf_prefix)
        name = self._cfg.get_val(pre)
        if not name:
            return None

        try:
            props = module_obj_factory (name, self._cfg, pre, self.submit_url)
        except IOError:
            return None
        return props

    def _render_rule (self):
        pre = "%s!match"%(self._conf_prefix)

        if self._cfg.get_val(pre) == 'default':
            return self.Dialog (DEFAULT_RULE_WARNING, 'important-information')

        # Change the rule type
        table = TableProps()
        e = self.AddPropOptions_Reload (table, "Rule Type", pre, RULES, "")
        return str(table) + e

    def _render_expiration (self):
        txt = ''
        pre = "%s!expiration"%(self._conf_prefix)

        table = TableProps()
        self.AddPropOptions_Ajax (table, "Expiration", pre, EXPIRATION_TYPE, NOTE_EXPIRATION)

        exp = self._cfg.get_val(pre)
        if exp == 'time':
            self.AddPropEntry (table, 'Time to expire', '%s!time'%(pre), NOTE_EXPIRATION_TIME)

        txt += str(table)
        return txt

    def _render_security (self):
        pre = self._conf_prefix

        self.AddHelp (('cookbook_authentication', 'Authentication'))

        txt   = "<h2>Access Restrictions</h2>"
        table = TableProps()
        self.AddPropCheck (table, 'Only https', '%s!only_secure'%(pre), False, NOTE_HTTPS_ONLY)
        self.AddPropEntry (table, 'Allow From',  '%s!allow_from' %(pre), NOTE_ALLOW_FROM)
        txt += self.Indent(table)

        txt += "<h2>Authentication</h2>"
        table = TableProps()
        e = self.AddPropOptions_Reload (table, 'Validation Mechanism', '%s!auth'%(pre),
                                        modules_available(VALIDATORS), NOTE_VALIDATOR)
        txt += self.Indent (table)
        txt += e

        self.AddHelps (module_get_help (self._cfg.get_val('%s!auth'%(pre))))

        return txt

    def _render_encoding (self):
        txt = ''
        pre = "%s!encoder"%(self._conf_prefix)
        encoders = modules_available(ENCODERS)

        for e in encoders:
            self.AddHelp (('modules_encoders_%s'%(e[0]),
                           '%s encoder' % (e[1])))

        txt += "<h2>Information Encoders</h2>"
        table = TableProps()
        for e,e_name in encoders:
            note = "Use the %s encoder whenever the client requests it." % (e_name)
            self.AddPropCheck (table, "Allow %s"%(e_name), "%s!%s"%(pre,e), False, note)

        txt += self.Indent(table)
        return txt
