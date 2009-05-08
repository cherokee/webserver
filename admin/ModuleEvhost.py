from Form import *
from Table import *
from Module import *
import validations

# For gettext
N_ = lambda x: x

NOTE_CHECK_DROOT = N_("Check the dynamically generated Document Root, and use the general Document Root if it doesn't exist.")
NOTE_REHOST      = N_("The Document Root directory will be built dynamically. The following variables are accepted:<br/>") +\
                   "${domain}, ${tld}, ${domain_no_tld}, ${subdomain1}, ${subdomain2}."

DATA_VALIDATION = [
]

class ModuleEvhost (Module, FormHelper):
    PROPERTIES = [
        'tpl_document_root',
        'check_document_root'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'evhost', cfg)
        Module.__init__ (self, 'evhost', cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''

        table = TableProps()
        self.AddPropEntry (table, _("Dynamic Document Root"), '%s!tpl_document_root'%(self._prefix), _(NOTE_REHOST))
        self.AddPropCheck (table, _("Check Document Root"),   '%s!check_document_root' %(self._prefix), True,  _(NOTE_CHECK_DROOT))
        txt += self.Indent(table)

        return txt

    def _op_apply_changes (self, uri, post):
        checkboxes = ['check_document_root']
        self.ApplyChangesPrefix (self._prefix, checkboxes, post, DATA_VALIDATION)
