from Form import *
from Table import *
import validations

from ModuleHandler import *
from ModuleFile import *

HELPS = [
    ('modules_handlers_secdownload', "Hidden Download")
]

DATA_VALIDATION = [
    ('vserver!.+?!rule!.+?!document_root',   (validations.is_local_dir_exists, 'cfg')),
    ('vserver!.+?!rule!.+?!handler!timeout', validations.is_number_gt_0),
]

NOTE_SECRET  = "Shared secret between the server and the script."
NOTE_TIMEOUT = "How long the download will last accessible - in seconds. Default: 60."


class ModuleSecdownload (ModuleHandler):
    PROPERTIES = ModuleFile.PROPERTIES + [
        'secret', 'timeout'
    ]

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'secdownload', cfg, prefix, submit_url)
        self._file = ModuleFile (cfg, prefix, submit_url)

    def _op_render (self):
        txt = ''

        # Local properties
        table = TableProps()
        self.AddPropEntry (table, 'Secret',  "%s!secret"  % (self._prefix), NOTE_SECRET)
        self.AddPropEntry (table, 'Timeout', "%s!timeout" % (self._prefix), NOTE_TIMEOUT)

        txt = '<h2>Covering parameters</h2>'
        txt += self.Indent(table)

        # Copy errors to the modules, 
        # they may need to print them
        self._copy_errors (self, self._file)

        txt += self._file._op_render()
        return txt

    def _op_apply_changes (self, uri, post):
        # Secret
        self.Validate_NotEmpty (post, '%s!secret'%(self._prefix), "A shared-secret must be defined")

        # Special case: DocumentRoot
        pre = '!'.join(self._prefix.split('!')[:-1])
        self.Validate_NotEmpty (post, '%s!document_root'%(pre), "It can't be empty")

        # Apply the changes
        self._file._op_apply_changes (uri, post)

        # Copy errors from the child modules
        self._copy_errors (self._file,    self)

        self.ApplyChangesPrefix (self._prefix, [], post, DATA_VALIDATION)

    def _copy_errors (self, _from, _to):
        for e in _from.errors:
            _to.errors[e] = _from.errors[e]
