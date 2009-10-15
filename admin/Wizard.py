import os, imp, sys
from util import *
from Page import *

# For gettext
N_ = lambda x: x

NOTE_DUP_LOGS = N_("Use the same logging configuration as one of the other virtual servers.")

WIZARD_GROUP_MISC       = N_('Misc')
WIZARD_GROUP_CMS        = N_('CMS')
WIZARD_GROUP_TASKS      = N_('Tasks')
WIZARD_GROUP_PLATFORM   = N_('Platforms')
WIZARD_GROUP_LANGS      = N_('Languages')
WIZARD_GROUP_MANAGEMENT = N_('Management')
WIZARD_GROUP_DB         = N_('Database')

USUAL_STATIC_FILES = ['/favicon.ico', '/robots.txt', '/crossdomain.xml']

class Wizard:
    def __init__ (self, cfg, pre=None):
        self.name    = _("Unknown wizard")
        self._cfg    = cfg
        self._pre    = pre
        self.no_show = None
        self.group   = _(WIZARD_GROUP_MISC)

    def show (self):
        assert (False);

    def run (self, uri, post):
        self.show()
        return self._run(uri, post)

    def report_error (self, msg, desc=''):
        content  = '<h1>Wizard: %s</h1>' % (self.name)
        content += '<div class="dialog-error">%s</div>' % (msg)
        content += '<div class="indented">%s</div>'% (desc)

        page = PageMenu ("wizard_error", self._cfg)
        page.AddMacroContent ('title', "%s error"%(self.name))
        page.AddMacroContent ('content', content)
        return page.Render()

    # Utilities
    #
    def _apply_cfg_chunk (self, config_txt):
        lines = config_txt.split('\n')
        lines = filter (lambda x: len(x) and x[0] != '#', lines)

        for line in lines:
            line = line.strip()
            left, right = line.split (" = ", 2)
            self._cfg[left] = right

class WizardManager:
    def __init__ (self, cfg, _type, pre):
        self.type = _type
        self._cfg = cfg
        self._pre = pre

    def get_available (self):
        wizards = []

        for fname in os.listdir('.'):
            # Filter names
            if not fname.startswith("Wizard_") or \
               not fname.endswith('.py'):
                continue

            # Load the module source file
            name = fname[:-3]
            mod = imp.load_source (name, fname)
            sys.modules[name] = mod

            # Find a suitable class
            for symbol in dir(mod):
                if not symbol.startswith ("Wizard_%s_"%(self.type)):
                    continue

                sname = name.replace("Wizard_", '')
                if sname in [x[0] for x in wizards]:
                    continue

                src = "wizard = mod.%s (self._cfg, self._pre)" % (symbol)
                exec(src)
                wizards.append ((sname, wizard))

        return wizards

    def _render_group (self, url_pre, wizards_all, group):
        txt = ''

        wizards = filter (lambda x: x[1].group == group, wizards_all)
        if not wizards: return ''

        for tmp in wizards:
            name, wizard = tmp
            show = wizard.show()
            img  = "/static/images/wizards/%s" % (wizard.ICON)

            txt += 'cherokeeWizards["%s"]["%s"] = {"title": "%s", "desc": "%s", "img": "%s", ' % (group, name, name, wizard.DESC, img) 

            if not show:
                txt += '"show": false, "no_show": "%s" }\n' % (wizard.no_show) 
            else:
                url = "%s/wizard/%s" % (url_pre, name)
                txt += '"show": true, "url": "%s" }\n' % (url) 

        return txt

    def render (self, url_pre):
        wizards = self.get_available()
        if not wizards:
            return ''

        # Group names
        group_names = [(w[1].group, None) for w in wizards]
        boxes = []
        for group in dict(group_names).keys():
            boxes += [(group, self._render_group (url_pre, wizards, group))]
        boxes.sort (lambda x,y: len(y[1]) - len(x[1]))

        # Table of wizard groups
        added = 0
        txt = '<script type="text/javascript">'
        txt += 'var cherokeeWizards = {} \n'
        for name, t in boxes:
            if not t:
                continue
            txt += 'cherokeeWizards["%s"] = {} \n %s' % (name, t)
        txt += '$(document).ready(function() { wizardRender(); });'
        txt += '</script>'
        txt += '<script type="text/javascript" src="/static/js/wizards.js"></script>'

        return txt

    def load (self, name):
        fname = 'Wizard_%s.py' % (name)
        mod = imp.load_source (name, fname)
        sys.modules[name] = mod

        src = "wizard = mod.Wizard_%s_%s (self._cfg, self._pre)" % (self.type, name)
        exec(src)

        return wizard


class WizardPage (PageMenu, FormHelper, Wizard):
    def __init__ (self, cfg, pre, submit, id, title, group):
        Wizard.__init__     (self, cfg, pre)
        PageMenu.__init__   (self, id, cfg, [])
        FormHelper.__init__ (self, id, cfg)

        self.submit_url = submit
        self.title      = title
        self.group      = group

    # Virtual Methods
    #
    def _render_content (self):
        assert (False)
    def _op_apply (self):
        assert (False)

    # Methods
    #
    def _op_render (self, url_pre):
        content = self._render_content (url_pre)
        self.AddMacroContent ('title', self.title)
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _run (self, url_pre, post):
        if post.get_val('is_submit'):
            re = self._op_apply (post)
            if self.has_errors():
                return self._op_render (url_pre)
        else:
            return self._op_render (url_pre)

    # Utilities
    #
    def _cfg_store_post (self, post):
        for key in post:
            if key.startswith('tmp!'):
                self._cfg[key] = post[key][0]

    def _cfg_clean_values (self, post):
        for key in post:
            if key.startswith('tmp!'):
                del(self._cfg[key])

    # Common pieces
    #
    def _common_add_logging (self):
        txt  = ''
        logs = [('', 'No Logs')]

        for v in self._cfg.keys('vserver'):
            pre  = 'vserver!%s!logger' % (v)
            log  = self._cfg.get_val(pre)
            nick = self._cfg.get_val('vserver!%s!nick'%(v))
            if not log: continue
            logs.append ((pre, '%s (%s)'%(nick, log)))

        table = TableProps()
        self.AddPropOptions (table, _('Same logs as vserver'), 'tmp!wizard!logs_as_vsrv', logs, _(NOTE_DUP_LOGS))
        txt += self.Indent(table)

        return txt

    def _common_apply_logging (self, post, vsrv_pre):
        logging_as = post.pop('tmp!wizard!logs_as_vsrv')
        if not logging_as:
            return

        self._cfg.clone (logging_as, '%s!logger'%(vsrv_pre))

    def _common_add_usual_static_files (self, rule_pre):
        self._cfg['%s!match'%(rule_pre)]   = 'fullpath'
        self._cfg['%s!handler'%(rule_pre)] = 'file'

        n = 1
        for file in USUAL_STATIC_FILES:
            self._cfg['%s!match!fullpath!%d'%(rule_pre,n)] = file
            n += 1

