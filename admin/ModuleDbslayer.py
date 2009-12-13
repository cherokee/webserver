from Form import *
from Table import *
from ModuleHandler import *
from validations import *
from consts import *

# For gettext
N_ = lambda x: x

from ModuleBalancer import NOTE_BALANCER

LANG_OPTIONS = [
    ('json',   'JSON'),
    ('python', 'Python'),
    ('php',    'PHP'),
    ('ruby',   'Ruby')
]

NOTE_LANG     = N_("Language from which the information will be consumed.")
NOTE_USER     = N_("User to access the database.")
NOTE_PASSWORD = N_("Password for the user accessing the database.")
NOTE_DB       = N_("Database to connect to.")

HELPS = [
    ('modules_handlers_dbslayer', N_("MySQL balancing")),
    ('cookbook_dbslayer', N_("DB balancig recipe"))
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

        txt += '<h2>%s</h2>' % (_('Serialization'))
        table = TableProps()
        self.AddPropOptions_Reload_Plain (table, _("Language"), "%s!lang" % (self._prefix), LANG_OPTIONS, _(NOTE_LANG))
        self.AddPropEntry   (table, _("DB User"),      "%s!user" % (self._prefix),     _(NOTE_USER))
        self.AddPropEntry   (table, _("DB Password"),  "%s!password" % (self._prefix), _(NOTE_PASSWORD))
        self.AddPropEntry   (table, _("Data Base"),    "%s!db" % (self._prefix),       _(NOTE_DB), optional=True)
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_('Data base balancing'))
        table = TableProps()
        prefix = "%s!balancer" % (self._prefix)
        e = self.AddPropOptions_Reload_Module (table, _("Balancer"), prefix,
                                              modules_available(BALANCERS), _(NOTE_BALANCER))
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
