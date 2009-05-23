import os, imp, sys

class Wizard:
    def __init__ (self, cfg, pre=None):
        self.name    = "Unknown wizard"
        self._cfg    = cfg
        self._pre    = pre
        self.no_show = None

    def show (self):
        assert (False);

    def run (self, uri, post):
        self.show()
        return self._run(uri, post)

    def error (self, msg):
        print "ERROR!! %s" % (msg)

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

    def render (self, url_pre):
        txt     = ''
        no_show = ''

        wizards = self.get_available()
        if not wizards:
            return ''

        for tmp in wizards:
            name, wizard = tmp
            show = wizard.show()
            img  = "/static/images/wizards/%s" % (wizard.ICON)

            if not show:
                txt += '<tr><td rowspan="2"><img src="%s" /></td><td><b>%s</b> (disabled)</td></tr>' % (img, name)
                txt += '<tr><td>%s <!-- NOT ADDED BECAUSE: %s --></td></tr>' % (wizard.DESC, wizard.no_show)
            else:
                url = "%s/wizard/%s" % (url_pre, name)
                txt += '<tr><td rowspan="2"><a href="%s"><img src="%s" /></a></td><td><b>%s</b></td></tr>' % (url, img, name)
                txt += '<tr><td>%s</td></tr>' % (wizard.DESC)

        return '<table>%s</table>' %(txt)

    def load (self, name):
        fname = 'Wizard_%s.py' % (name)
        mod = imp.load_source (name, fname)
        sys.modules[name] = mod

        src = "wizard = mod.Wizard_%s_%s (self._cfg, self._pre)" % (self.type, name)
        exec(src)
        
        return wizard
