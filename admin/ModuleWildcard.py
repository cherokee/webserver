from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_WILDCARD = N_("Accepted host name. Wildcard characters (* and ?) are allowed. Eg: *example.com")
WARNING_EMPTY = N_("At least one wildcard string must be defined.")

class ModuleWildcard (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'wildcard', cfg)
        Module.__init__ (self, 'wildcard', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''
        pre = '%s!domain'%(self._prefix)
        cfg_domains = self._cfg[pre]

        available = "1"

        txt += "<h2>%s</h2>" % (_('Accepted domains'))
        if cfg_domains and \
           cfg_domains.has_child():
            table = Table(2,1)
            table += (_('Domain pattern'), '')

            # Build list
            for i in cfg_domains:
                domain = cfg_domains[i].value
                cfg_key = "%s!%s" % (pre, i)
                en = self.InstanceEntry (cfg_key, 'text')
                link_del = self.AddDeleteLink ('/ajax/update', cfg_key)
                table += (en, link_del)

            txt += self.Indent(table)
            txt += "<br />"
        else:
            txt += self.Dialog(WARNING_EMPTY, 'warning')

        # Look for firs available
        i = 1
        while cfg_domains:
            if not cfg_domains[str(i)]:
                available = str(i)
                break
            i += 1

        table = TableProps()
        self.AddPropEntry (table, _('New host name'), '%s!%s'%(pre, available), _(NOTE_WILDCARD))
        txt += "<h3>%s</h3>" % (_('Add new'))
        txt += self.Indent(table)

        return txt
