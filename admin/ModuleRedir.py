from Form import *
from Table import *
from ModuleHandler import *
from consts import *
import validations

# For gettext
N_ = lambda x: x

NOTE_SHOW         = N_("Defines whether the redirection will be seen by the client.")
NOTE_REGEX        = N_('Regular expression. Check out the <a target="_blank" href="http://perldoc.perl.org/perlre.html">Reference</a>.')
NOTE_SUBSTITUTION = N_("Target address. It can use Regular Expression substitution sub-strings.")

DATA_VALIDATION = [
    ('vserver!.+?!rule!.+?!handler!rewrite|.+?|regex', validations.is_regex),
    ('rewrite_new_regex', validations.is_regex)
]

HELPS = [
    ('modules_handlers_redir',              N_("Redirections")),
    ('http://perldoc.perl.org/perlre.html', N_("Regular Expressions"))
]

class ModuleRedir (ModuleHandler):
    PROPERTIES = [
        "rewrite"
    ]

    def __init__ (self, cfg, prefix, submit_url):
        ModuleHandler.__init__ (self, 'redir', cfg, prefix, submit_url)
        self.show_document_root = False

    def _op_render (self):
        cfg_key = "%s!rewrite" % (self._prefix)
        cfg     = self._cfg[cfg_key]
        txt     = ''

        # Current rules
        if cfg and cfg.has_child():
            table = Table (4,1)
            table += (_('Type'), _('Regular Expression'), _('Substitution'), '')

            for rule in cfg:
                cfg_key_rule = "%s!%s" % (cfg_key, rule)

                show, trash = self.InstanceOptions ('%s!show'%(cfg_key_rule), REDIR_SHOW)
                regex       = self.InstanceEntry('%s!regex' % (cfg_key_rule), 'text', size=25)
                substring   = self.InstanceEntry('%s!substring' % (cfg_key_rule), 'text', size=25)
                link_del = self.AddDeleteLink ('/ajax/update', cfg_key_rule)
                table += (show, regex, substring, link_del)

            txt += "<h3>%s</h3>" % (_('Rule list'))
            txt += self.Indent(table)

        # Add new rule
        table = TableProps()
        self.AddPropOptions (table, _('Show'), "rewrite_new_show", REDIR_SHOW, _(NOTE_SHOW), noautosubmit=True)
        self.AddPropEntry   (table, _('Regular Expression'), 'rewrite_new_regex', _(NOTE_REGEX), noautosubmit=True, optional=True)
        self.AddPropEntry   (table, _('Substitution'), 'rewrite_new_substring', _(NOTE_SUBSTITUTION), req=True)

        txt += "<h2>%s</h2>" % (_('Add new rule'))
        txt += self.Indent(table)

        return txt

    def __find_name (self):
        i = 1
        while True:
            key = "%s!rewrite!%d" % (self._prefix, i)
            tmp = self._cfg[key]
            if not tmp:
                return str(i)
            i += 1

    def _op_apply_changes (self, uri, post):
        self.ValidateChange_SingleKey ('rewrite_new_regex', post, DATA_VALIDATION)
        if self.has_errors():
            return self._op_render()

        regex  = post.pop('rewrite_new_regex')
        substr = post.pop('rewrite_new_substring')
        show   = post.pop('rewrite_new_show')

        if regex or substr:
            pre = "%s!rewrite!%s" % (self._prefix, self.__find_name())

            self._cfg['%s!show'%(pre)] = show
            if regex:
                self._cfg['%s!regex'%(pre)] = regex
            if substr:
                self._cfg['%s!substring'%(pre)] = substr

        self.ApplyChangesPrefix (self._prefix, [], post, DATA_VALIDATION)
