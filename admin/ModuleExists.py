from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_EXISTS  = N_("Comma separated list of files. Rule applies if one exists.")
NOTE_IOCACHE = N_("Uses cache during file detection. Disable if directory contents change frequently. Enable otherwise.")
NOTE_ANY     = N_("Match the request if any file exists.")

class ModuleExists (Module, FormHelper):
    validation = [('tmp!new_rule!value', validations.is_safe_id_list)]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'exists', cfg)
        Module.__init__ (self, 'exists', cfg, prefix, submit_url)

        # Special case: there is a check in the rule
        self.checks = ['%s!iocache'%(self._prefix),
                       '%s!match_any'%(self._prefix)]

    def _op_render (self):
        table = TableProps()
        if self._prefix.startswith('tmp!'):
            self.AddPropEntry (table, _('Files'), '%s!value'%(self._prefix), _(NOTE_EXISTS))
        else:
            self.AddPropCheck (table, _('Match any file'), '%s!match_any'%(self._prefix), False, _(NOTE_ANY))
            if not int(self._cfg.get_val ('%s!match_any'%(self._prefix), '0')):
                self.AddPropEntry (table, _('Files'), '%s!exists'%(self._prefix), _(NOTE_EXISTS))
            self.AddPropCheck (table, _('Use I/O cache'), '%s!iocache'%(self._prefix), False, _(NOTE_IOCACHE))
        return str(table)

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print _("ERROR, a 'value' entry is needed!")

        exts = values['value']
        self._cfg['%s!exists'%(self._prefix)] = exts

    def get_name (self):
        if int(self._cfg.get_val ('%s!match_any'%(self._prefix), 0)):
            return _("Any file")
        return self._cfg.get_val ('%s!exists'%(self._prefix))

    def get_type_name (self):
        return _("File Exists")
