from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_EXISTS      = N_("Comma separated list of files. Rule applies if one exists.")
NOTE_IOCACHE     = N_("Uses cache during file detection. Disable if directory contents change frequently. Enable otherwise.")
NOTE_ANY         = N_("Match the request if any file exists.")
NOTE_ONLY_FILES  = N_("Only match regular files. If unset directories will be matched as well.")
NOTE_INDEX_FILES = N_("If a directory is request, check the index files inside it.")

OPTIONS = [('0', _('Match a specific list of files')),
           ('1', _('Match any file'))]

class ModuleExists (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'exists', cfg)
        Module.__init__ (self, 'exists', cfg, prefix, submit_url)

        # Special case: there is a check in the rule
        self.checks = ['%s!iocache'%(self._prefix),
                       '%s!match_any'%(self._prefix), 
                       '%s!match_only_files'%(self._prefix),
                       '%s!match_index_files'%(self._prefix)]

        self.validation = [('tmp!new_rule!value',       validations.is_safe_id_list),
                           ('%s!exists'%(self._prefix), validations.is_safe_id_list)]

    def _op_render (self):
        if self._prefix.startswith('tmp!'):
            return self._render_new_entry()

        return self._render_modify_entry()

    def _render_new_entry (self):
        specific_file = not int(self._cfg.get_val('%s!match_any'%(self._prefix), '0'))

        table = TableProps()
        self.AddPropOptions_Reload (table, _("Match type"), '%s!match_any'%(self._prefix), OPTIONS, "")
        if specific_file:
            self.AddPropEntry (table, _('Files'), '%s!value'%(self._prefix), _(NOTE_EXISTS))

        txt = str(table)
        if not specific_file:
            txt += '<div align="right"><input type="submit" value="%s" /></div>' % (_('Add'))
            txt += '<input type="hidden" name="tmp!new_rule!bypass_value_check" value="1" />'

        return txt

    def _render_modify_entry (self):
        specific_file = not int(self._cfg.get_val ('%s!match_any'%(self._prefix), '0'))

        table = TableProps()
        self.AddPropCheck (table, _('Match any file'), '%s!match_any'%(self._prefix), False, _(NOTE_ANY))
        if specific_file:
            self.AddPropEntry (table, _('Files'), '%s!exists'%(self._prefix), _(NOTE_EXISTS))
        self.AddPropCheck (table, _('Use I/O cache'),    '%s!iocache'%(self._prefix), False, _(NOTE_IOCACHE))
        self.AddPropCheck (table, _('Only match files'), '%s!match_only_files'%(self._prefix),  True, _(NOTE_ONLY_FILES))
        self.AddPropCheck (table, _('If dir, check index files'),'%s!match_index_files'%(self._prefix), True, _(NOTE_INDEX_FILES))
        return str(table)

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value') and \
           not values.has_key('bypass_value_check'):
            print _("ERROR, a 'value' entry is needed!")

        any = values['match_any']
        if int(any):
            self._cfg['%s!match_any'%(self._prefix)] = any
        else:
            exts = values['value']
            self._cfg['%s!exists'%(self._prefix)] = exts

    def get_name (self):
        if int(self._cfg.get_val ('%s!match_any'%(self._prefix), 0)):
            return _("Any file")
        return self._cfg.get_val ('%s!exists'%(self._prefix))

    def get_type_name (self):
        return _("File Exists")
