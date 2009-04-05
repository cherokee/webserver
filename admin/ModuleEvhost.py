from Form import *
from Table import *
from Module import *
import validations

NOTE_REHOST = "The document root will be built dynamically. The following variables are accepted:<br/>" +\
    "${domain}, ${tld}, ${domain_no_tld}, ${subdomain1}, ${subdomain2}."

class ModuleEvhost (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'evhost', cfg)
        Module.__init__ (self, 'evhost', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''

        table = TableProps()
        self.AddPropEntry (table, 'Dynamic Document Root', '%s!tpl_document_root'%(self._prefix), NOTE_REHOST)
        txt += self.Indent(table)

        return txt
