from Form import *
from Table import *
from Module import *
import validations

METHODS = [
    ('',            'Choose'),
    ('get',         'GET'),
    ('post',        'POST'),
    ('head',        'HEAD'),
    ('put',         'PUT'),
    ('options',     'OPTIONS'),
    ('delete',      'DELETE'),
    ('trace',       'TRACE'),
    ('connect',     'CONNECT'),
    ('copy',        'COPY'),
    ('lock',        'LOCK'),
    ('mkcol',       'MKCOL'),
    ('move',        'MOVE'),
    ('notify',      'NOTIFY'),
    ('poll',        'POLL'),
    ('propfind',    'PROPFIND'),
    ('proppatch',   'PROPPATCH'),
    ('search',      'SEARCH'),
    ('subscribe',   'SUBSCRIBE'),
    ('unlock',      'UNLOCK'),
    ('unsubscribe', 'UNSUBSCRIBE')
]

NOTE_METHOD  = "The HTTP method that should match this rule."


class ModuleMethod (Module, FormHelper):
    validation = [('tmp!new_rule!value', validations.is_safe_id_list)]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'method', cfg)
        Module.__init__ (self, 'method', cfg, prefix, submit_url)

    def _op_render (self):
        table = TableProps()

        if self._prefix.startswith('tmp!'):
            self.AddPropOptions (table, 'Method', '%s!value'%(self._prefix), \
                                 METHODS, NOTE_METHOD)
        else:
            self.AddPropOptions (table, 'Method', '%s!method'%(self._prefix), \
                                 METHODS[1:], NOTE_METHOD)

        return str(table)

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print "ERROR, a 'value' entry is needed!"

        exts = values['value']
        self._cfg['%s!method'%(self._prefix)] = exts

    def get_name (self):
        return self._cfg.get_val ('%s!method'%(self._prefix))

    def get_type_name (self):
        return "HTTP method"
