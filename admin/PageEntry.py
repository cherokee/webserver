from Page import *
from Form import *
from Table import *
from Entry import *
from VirtualServer import *

HANDLERS = [
    ('common',      'List & Send'),
    ('file',        'Static content'),
    ('dirlist',     'Only listing'),
    ('cgi',         'CGI'),
    ('fcgi',        'FastCGI'),
    ('scgi',        'SCGI'),
    ('server_info', 'Server Info')
]

VALIDATORS = [
    ('',         'No authentication'),
    ('plain',    'Plain text file'),
    ('htpasswd', 'Htpasswd file'),
    ('htdigest', 'Htdigest file'),
    ('ldap',     'LDAP server'),
    ('mysql',    'MySQL server')
]

ENTRY_TYPES = [
    ('directory',  'Directory'),
    ('extensions', 'Extensions'),
    ('request',    'Regular Expression')
]


class PageEntry (PageMenu):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, cfg)
        self._id         = 'entry'
        self._priorities = None

    def _op_render (self):
        raise "no"

    def _op_handler (self, uri, post):
        assert (len(uri) > 1)
        assert ("prio" in uri)

        temp = uri.split('/')
        host = temp[1]
        prio = temp[3]
        
        self._priorities  = VServerEntries (host, self._cfg)
        self._entry       = self._priorities[prio]
        self._conf_prefix = 'vserver!%s!%s!%s' % (host, self._entry[0], self._entry[1])

        # Render page
        title   = self._get_title()
        content = self._render_guts()

        self.AddMacroContent ('title',   title)
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _get_title (self):
        for n in range(len(ENTRY_TYPES)):
            if ENTRY_TYPES[n][0] == self._entry[0]:
                return "%s: %s" % (ENTRY_TYPES[n][1], self._entry[1])

    def _render_guts (self):
        pre = self._conf_prefix
        txt = '<h1>%s</h1>' % (self._get_title())

        table = Table(2)
        self.AddTableOptions (table, 'Handler',       '%s!handler'%(pre), HANDLERS)
        self.AddTableEntry   (table, 'Document Root', '%s!document_root'%(pre))
        txt += str(table)

        txt += '<h2>Authentication</h2>'
        txt += self._render_auth ()

        return txt

    def _render_auth (self):
        pre = self._conf_prefix

        table = Table(2)
        self.AddTableOptions (table, 'Validator', '%s!auth'%(pre), VALIDATORS)

        return str(table)

