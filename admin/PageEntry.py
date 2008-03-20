import validations

from Page import *
from Form import *
from Table import *
from Entry import *
from VirtualServer import *
from Module import *
from consts import *

DATA_VALIDATION = [
    ("vserver!.*?!(directory|extensions|request)!.*?!document_root", validations.is_local_dir_exists),
    ("vserver!.*?!(directory|extensions|request)!.*?!allow_from",    validations.is_ip_or_netmask_list)
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

        if not self._is_userdir:
            self._host        = temp[1]
            self._prio        = temp[3]        
            self._priorities  = VServerEntries (self._host, self._cfg)
            self._entry       = self._priorities[self._prio]
            url = '/vserver/%s/prio/%s' % (self._host, self._prio)
        else:
            self._host        = temp[1]
            self._prio        = temp[4]        
            self._priorities  = VServerEntries (self._host, self._cfg, user_dir=True)
            self._entry       = self._priorities[self._prio]
            url = '/vserver/%s/userdir/prio/%s' % (self._host, self._prio)

        # Set the submit URL
        self.set_submit_url (url)

    def _op_handler (self, uri, post):
        # Parse the URI
        self._parse_uri (uri)
        
        # Entry not found
        if not self._entry:
            return "/vserver/%s" % (self._host)

        if not self._is_userdir:
            self._conf_prefix = 'vserver!%s!%s!%s' % (self._host, self._entry[0], self._entry[1])
        else:
            self._conf_prefix = 'vserver!%s!user_dir!%s!%s' % (self._host, self._entry[0], self._entry[1])

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

        for n in range(len(ENTRY_TYPES)):
            if ENTRY_TYPES[n][0] == self._entry[0]:
                txt += "%s: %s" % (ENTRY_TYPES[n][1], self._entry[1])
                return txt

    def _render_guts (self):
        pre = self._conf_prefix
        txt = '<h1>%s</h1>' % (self._get_title (html=True))
        tmp = ''

        table = Table(2)
        e = self.AddTableOptions_Reload (table, 'Handler', '%s!handler'%(pre), HANDLERS)
        self.AddTableEntry (table, 'Document Root', '%s!document_root'%(pre))
       
        tmp += str(table)

        tmp += '<h2>Handler properties</h2>'
        tmp += e

        tmp += '<h2>Security</h2>'
        tmp += self._render_security ()
        txt += self.Indent(tmp)

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

    def _render_security (self):
        pre = self._conf_prefix

        txt   = ""
        table = Table(2)
        self.AddTableCheckbox (table, 'Only https', '%s!only_secure'%(pre), False)
        self.AddTableEntry    (table, 'Allow From',  '%s!allow_from' %(pre))

        e = self.AddTableOptions_Reload (table, 'Authentication', '%s!auth'%(pre), VALIDATORS)
                                                     
        txt += str(table) + e
        return txt

