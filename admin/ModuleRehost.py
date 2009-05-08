from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_REHOST   = N_("Regular Expression against which the hosts be Host name will be compared.")
WARNING_EMPTY = N_("At least one Regular Expression string must be defined.")

class ModuleRehost (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'rehost', cfg)
        Module.__init__ (self, 'rehost', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''
        pre = '%s!regex'%(self._prefix)
        cfg_domains = self._cfg[pre]

        available = "1"

        txt += "<h2>%s</h2>" % (_('Regular Expressions'))
        if cfg_domains and \
           cfg_domains.has_child():
            table = Table(2,1)
            table += (_('Regular Expressions'), '')

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
        self.AddPropEntry (table, _('New Regular Expression'), '%s!%s'%(pre, available), _(NOTE_REHOST))
        txt += "<h3>%s</h3>" % (_('Add new'))
        txt += self.Indent(table)

        return txt
