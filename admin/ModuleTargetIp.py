from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_ADDRESS  = N_("IP or Subnet of the NIC that accepted the request. Example: ::1, or 10.0.0.0/8")
WARNING_EMPTY = N_("At least one IP or Subnet entry must be defined.")

DATA_VALIDATION = [
    ('vserver!.+?!match!to!.+', validations.is_ip_or_netmask)
]

class ModuleTargetIp (Module, FormHelper):
    PROPERTIES = ['ip']

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'target_ip', cfg)
        Module.__init__ (self, 'target_ip', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''
        pre = '%s!to'%(self._prefix)
        cfg_addresses = self._cfg[pre]

        available = "1"

        txt += "<h2>%s</h2>" % (_('Accepted Server IP addresses and subnets'))
        if cfg_addresses and \
           cfg_addresses.has_child():
            table = Table(2,1)
            table += (_('IP or Subnet'), '')

            # Build list
            for i in cfg_addresses:
                domain = cfg_addresses[i].value
                cfg_key = "%s!%s" % (pre, i)
                en = self.InstanceEntry (cfg_key, 'text')
                if len(cfg_addresses.keys()) >= 2:
                    link_del = self.AddDeleteLink ('/ajax/update', cfg_key)
                else:
                    link_del = ''
                table += (en, link_del)

            txt += self.Indent(table)
            txt += "<br />"
        else:
            txt += self.Dialog(WARNING_EMPTY, 'warning')

        # Look for firs available
        i = 1
        while cfg_addresses:
            if not cfg_addresses[str(i)]:
                available = str(i)
                break
            i += 1

        table = TableProps()
        self.AddPropEntry (table, _('New IP/Subnet'), '%s!%s'%(pre, available), _(NOTE_ADDRESS))
        txt += "<h3>%s</h3>" % (_('Add new'))
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, [], post, DATA_VALIDATION)
