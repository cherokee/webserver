from Form import *
from Table import *
from Module import *
import validations

NOTE_WILDCARD = "Accepted host name. Wildcard characters (* and ?) are allowed. Eg: *example.com"
WARNING_EMPTY = "At least one wildcard string must be defined."

class ModuleWildcard (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'wildcard', cfg)
        Module.__init__ (self, 'wildcard', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''
        pre = '%s!domain'%(self._prefix)
        cfg_domains = self._cfg[pre]

        available = "1"

        txt += "<h2>Accepted domains</h2>"
        if cfg_domains and \
           cfg_domains.has_child():
            table = Table(2,1)
            table += ('Domain pattern', '')

            # Build list
            for i in cfg_domains:
                domain = cfg_domains[i].value
                cfg_key = "%s!%s" % (pre, i)
                en = self.InstanceEntry (cfg_key, 'text')
                js = "post_del_key('/ajax/update','%s');" % (cfg_key)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
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
        self.AddPropEntry (table, 'New host name', '%s!%s'%(pre, available), NOTE_WILDCARD)
        txt += "<h3>Add new</h3>"
        txt += self.Indent(table)

        return txt
