from Form import *
from Table import *
from ModuleHandler import *
from validations import *
from consts import *

from ModuleBalancer import NOTE_BALANCER

LANG_OPTIONS = [
    ('json',   'JSON'),
    ('python', 'Python'),
    ('php',    'PHP'),
    ('ruby',   'Ruby')
]

NOTE_LANG = "Language from which the information will be consumed."
NOTE_USER     = "User to access the database."
NOTE_PASSWORD = "Password for the user accessing the database."
NOTE_DB       = "Optionally specifies a database to connect to."

HELPS = [
    ('modules_handlers_dbslayer', "MySQL balancing")
]

class ModuleDbslayer (ModuleHandler):
    PROPERTIES = [
        'balancer'
    ]

    def __init__ (self, cfg, prefix, submit):
        ModuleHandler.__init__ (self, 'dbslayer', cfg, prefix, submit)
        self.show_document_root = False

    def _op_render (self):
        txt = ''

        txt += '<h2>Serialization</h2>'
        table = TableProps()
        self.AddPropOptions_Reload (table, "Language", "%s!lang" % (self._prefix), LANG_OPTIONS, NOTE_LANG)
        self.AddPropEntry   (table, "DB User",      "%s!user" % (self._prefix),     NOTE_USER)
        self.AddPropEntry   (table, "DB Password",  "%s!password" % (self._prefix), NOTE_PASSWORD)
        self.AddPropEntry   (table, "Data Base",    "%s!db" % (self._prefix),       NOTE_DB)
        txt += self.Indent(table)

        txt += '<h2>Data base balancing</h2>'
        table = TableProps()
        prefix = "%s!balancer" % (self._prefix)
        e = self.AddPropOptions_Reload (table, "Balancer", prefix, 
                                        modules_available(BALANCERS), NOTE_BALANCER)
        txt += self.Indent(str(table) + e)
        return txt

    def _op_apply_changes (self, uri, post):
        # Apply balancer changes
        pre = "%s!balancer" % (self._prefix)

        new_balancer = post.pop(pre)
        if new_balancer:
            self._cfg[pre] = new_balancer

        cfg  = self._cfg[pre]
        if cfg and cfg.value:
            name = cfg.value
            props = module_obj_factory (name, self._cfg, pre, self.submit_url)
            props._op_apply_changes (uri, post)

        # And apply the rest
        self.ApplyChangesPrefix (self._prefix, [], post)
