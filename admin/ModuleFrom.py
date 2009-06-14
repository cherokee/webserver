from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_FROM = N_("IP or SubNet to check the connection origin against.")

class ModuleFrom (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'from', cfg)
        Module.__init__ (self, 'from', cfg, prefix, submit_url)

        self.validation = [('tmp!new_rule!value', validations.is_ip_or_netmask),
                           ('%s!from!'%(prefix),  validations.is_ip_or_netmask)]

    def _op_render (self):
        if self._prefix.startswith('tmp!'):
            return self._op_render_new()
        else:
            return self._op_render_modify()

    def _op_render_new (self):
        table = TableProps()
        self.AddPropEntry (table, _('Connected from'), '%s!value'%(self._prefix), _(NOTE_FROM))
        return str(table)

    def _op_render_modify (self):
        txt = ''

        # Previous entries
        table = Table(2,1)
        table += (_('IP or SubNet'), '')

        n = 0
        keys = self._cfg.keys("%s!from"%(self._prefix))
        for k in keys:
            ip = self._cfg.get_val ('%s!from!%s'%(self._prefix, k))
            if len(keys) > 1:
                link_del = self.AddDeleteLink ('/ajax/update', "%s!from!%s"%(self._prefix, k))
            else:
                link_del = ''
            table += (ip, link_del)
            n = max(int(k), n)
            
        if n > -1:
            txt += '<h2>IPs and SubNets</h2>'
            txt += self.Indent(table)

        # New Entry
        table = TableProps()
        self.AddPropEntry (table, _('Add new entry'), '%s!from!%d'%(self._prefix, n+1), _(NOTE_FROM))
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print _("ERROR, a 'value' entry is needed!")

        exts = values['value']
        self._cfg['%s!from!1'%(self._prefix)] = exts

    def get_name (self):
        vals = []
        for k in self._cfg.keys('%s!from'%(self._prefix)):
            vals.append (self._cfg.get_val('%s!from!%s'%(self._prefix, k)))
        return ','.join(vals)

    def get_type_name (self):
        return _('Connected from')
