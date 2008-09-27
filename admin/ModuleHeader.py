from Form import *
from Table import *
from Module import *
import validations

NOTE_HEADER = "Header against which the regular expression will be evaluated."
NOTE_MATCH  = "Regular expression."

LENGHT_LIMIT = 10

HEADERS = [
    ('Accept-Encoding', 'Accept-Encoding'),
    ('Accept-Charset',  'Accept-Charset'),
    ('Accept-Language', 'Accept-Language'),
    ('Referer',         'Referer'),
    ('User-Agent',      'User-Agent')
]

class ModuleHeader (Module, FormHelper):
    validation = [('tmp!new_rule!match', validations.is_regex)]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'header', cfg)
        Module.__init__ (self, 'header', cfg, prefix, submit_url)

    def _op_render (self):
        table = TableProps()
        if self._prefix.startswith('tmp!'):
            self.AddPropOptions_Reload (table, 'Header', '%s!value'%(self._prefix), HEADERS, NOTE_HEADER)
        else:
            self.AddPropOptions_Reload (table, 'Header', '%s!header'%(self._prefix), HEADERS, NOTE_HEADER)
        self.AddPropEntry   (table, 'Regular Expression', '%s!match'%(self._prefix), NOTE_MATCH)
        return str(table)
        
    def apply_cfg (self, values):
        if values.has_key('value'):
            header = values['value']
            self._cfg['%s!match!header'%(self._prefix)] = header

        if values.has_key('match'):
            match = values['match']
            self._cfg['%s!match!match'%(self._prefix)] = match

    def get_name (self):
        header = self._cfg.get_val ('%s!match!header'%(self._prefix))
        if not header:
            return ''

        tmp  = self._cfg.get_val ('%s!match!match'%(self._prefix), '')
        if len(tmp) > LENGHT_LIMIT:
            return "%s (%s..)" % (header, tmp[:5])

        return "%s (%s)" % (header, tmp)

    def get_type_name (self):
        return self._id.capitalize()
