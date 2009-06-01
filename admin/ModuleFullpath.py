from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_FULLPATH = N_("Full path request to which content the configuration will be applied. (Eg: /favicon.ico)")

class ModuleFullpath (Module, FormHelper):
    validation = [('tmp!new_rule!value',                   validations.is_path),
                  ('vserver!.+!rule!.+!match!fullpath!.+', validations.is_path)]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'fullpath', cfg)
        Module.__init__ (self, 'fullpath', cfg, prefix, submit_url)

    def _op_render (self):
        if self._prefix.startswith('tmp!'):
            table = TableProps()
            self.AddPropEntry (table, _('Full Path'), '%s!value'%(self._prefix), _(NOTE_FULLPATH))
            return str(table)

        txt = '<h3>%s</h3>' % (_("Full Web Paths"))
        pre = '%s!fullpath'%(self._prefix)

        table = Table (2)
        first = True
        for k in self._cfg.keys(pre):
            fpath    = self.InstanceEntry('%s!%s' % (pre, k), 'text', size=40)
            if not first:
                link_del = self.AddDeleteLink ('/ajax/update', "%s!%s"%(pre,k))
            else:
                link_del = ''
            table += (fpath, link_del)
            first = False

        txt += self.Indent(table)
        txt += '<h3>%s</h3>' % (_("Add another path"))

        tmp = [int(x) for x in self._cfg.keys(pre)]
        if len(tmp):
            tmp.sort()
            next = tmp[-1]+1
        else:
            next = 1

        table = TableProps()
        self.AddPropEntry (table, _('Full Path'), '%s!%d'%(pre,next), _(NOTE_FULLPATH))
        txt += self.Indent(table)
        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print _("ERROR, a 'value' entry is needed!")

        fullpath = values['value']
        self._cfg['%s!fullpath!1'%(self._prefix)] = fullpath

    def get_name (self):
        txt = ''
        for k in self._cfg.keys('%s!fullpath'%(self._prefix)):
            d = self._cfg.get_val ('%s!fullpath!%s'%(self._prefix, k))
            txt += '%s, ' % (d)

        if len(txt):
            txt = txt[:-2]

        return txt

    def get_type_name (self):
        return self._id.capitalize()
