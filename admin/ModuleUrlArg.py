from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_ARGUMENT = N_("Argument name")
NOTE_REGEX    = N_("Regular Expression for the match")

OPTIONS = [('0', _('Match a specific argument')),
           ('1', _('Match any argument'))]


class ModuleUrlArg (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'url_arg', cfg)
        Module.__init__ (self, 'url_arg', cfg, prefix, submit_url)

    def _op_render (self):
        if self._prefix.startswith('tmp!'):
            return self._render_new_entry()

        return self._render_modify_entry()

    def _render_new_entry (self):
        specific_arg = not int(self._cfg.get_val('%s!match_any'%(self._prefix), '0'))

        table = TableProps()
        self.AddPropOptions_Reload_Plain (table, _("Match type"), '%s!match_any'%(self._prefix), OPTIONS, "")
        if specific_arg:
            self.AddPropEntry (table, _('Argument'), '%s!arg'%(self._prefix), _(NOTE_ARGUMENT), noautosubmit=True)
        self.AddPropEntry (table, _('Regular Expression'), '%s!value'%(self._prefix), _(NOTE_REGEX))
        return str(table)


    def _render_modify_entry (self):
        specific_arg = not int(self._cfg.get_val ('%s!match_any'%(self._prefix), '0'))

        table = TableProps()
        self.AddPropOptions_Reload_Plain (table, _("Match type"), '%s!match_any'%(self._prefix), OPTIONS, "")
        if specific_arg:
            self.AddPropEntry (table, _('Argument'), '%s!arg'%(self._prefix), _(NOTE_ARGUMENT))
        self.AddPropEntry (table, _('Regular Expression'), '%s!match'%(self._prefix), _(NOTE_REGEX))
        return str(table)


    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        any = values['match_any']

        self._cfg['%s!match_any'%(self._prefix)] = any
        self._cfg['%s!match'    %(self._prefix)] = values['value']

        if not int(any):
            self._cfg['%s!arg'  %(self._prefix)] = values['arg']

    def get_name (self):
        match = self._cfg.get_val ('%s!match'%(self._prefix))

        if int(self._cfg.get_val ('%s!match_any'%(self._prefix), 0)):
            return _("Any arg ~= %s" %(match))

        txt = self._cfg.get_val ('%s!arg'%(self._prefix))
        if not txt:
            return ''
        else:
            txt += " ~= %s" % (match)
        return txt

    def get_type_name (self):
        return _("URL arg")
