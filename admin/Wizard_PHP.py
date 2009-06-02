from config import *
from util import *
from Wizard import *

DEFAULT_BINS  = ['php-cgi', 'php']

DEFAULT_PATHS = ['/usr/bin', 
                 '/opt/php', 
                 '/usr/php/bin', 
                 '/usr/sfw/bin',
                 '/usr/gnu/bin',
                 '/opt/local/bin']

class Wizard_Rules_PHP (Wizard):
    UNIX_SOCK     = "/tmp/cherokee-php.socket"
    ICON          = "php.jpg"
    DESC          = "Configures PHP in the current Virtual Server. It will add a new .php extension is not present."
    
    def __init__ (self, cfg, pre):
        Wizard.__init__ (self, cfg, pre)
        self.name   = "Add PHP support"
        self.source = None

    def show (self):
        self.rule   = None
        self.source = None
        self.nick   = None

        # Find: Source
        self.source = cfg_source_find_interpreter (self._cfg, 'php-cgi')
        if not self.source:
            return True

        # Find: Extension 'php' rule using prev Source
        self.rule = cfg_vsrv_rule_find_extension (self._cfg, self._pre, 'php')
        if not self.rule:
            return True

        # Already configured
        self.nick = self._cfg.get_val ("%s!nick"%(self.source))
        self.no_show = "Already configured: nick=%s" % (self.nick)
        return False

    def _run (self, uri, post):
        def test_php_fcgi (path):
            f = os.popen("%s -v" % (path), 'r')
            output = f.read()
            try: f.close()
            except: pass
            return "(cgi-fcgi)" in output

        # Add a new Source, if needed
        if not self.source:
            php_path = path_find_binary (DEFAULT_BINS,
                                         extra_dirs  = DEFAULT_PATHS,
                                         custom_test = test_php_fcgi)
            if not php_path:
                desc = "<p>Looked for the binaries: %s.</p>" % (", ".join(DEFAULT_BINS))
                return self.report_error ("Couldn't find a suitable PHP interpreter.", desc)

            _, self.source = cfg_source_get_next (self._cfg)
            self._cfg['%s!nick' % (self.source)]        = 'PHP Interpreter'
            self._cfg['%s!type' % (self.source)]        = 'interpreter'
            self._cfg['%s!interpreter' % (self.source)] = '%s -b %s' % (php_path, self.UNIX_SOCK)
            self._cfg['%s!host' % (self.source)]        = self.UNIX_SOCK

            self._cfg['%s!env!PHP_FCGI_MAX_REQUESTS' % (self.source)] = "5000"
            self._cfg['%s!env!PHP_FCGI_CHILDREN' % (self.source)]     = "5"

        # Add a new Extension PHP rule
        if not self.rule:
            _, self.rule = cfg_vsrv_rule_get_next (self._cfg, self._pre)
            if not self.rule:
                return self.report_error ("Couldn't add a new rule.")
            
            src_num = self.source.split('!')[-1]

            self._cfg['%s!match' % (self.rule)]                     = 'extensions'
            self._cfg['%s!match!extensions' % (self.rule)]          = 'php'
            self._cfg['%s!match!final' % (self.rule)]               = '0'
            self._cfg['%s!handler' % (self.rule)]                   = 'fcgi'
            self._cfg['%s!handler!balancer' % (self.rule)]          = 'round_robin'
            self._cfg['%s!handler!balancer!source!1' % (self.rule)] = src_num
            self._cfg['%s!handler!error_handler' % (self.rule)]     = '1'
            self._cfg['%s!encoder!gzip' % (self.rule)]              = '1'

        # Check the Directory Index
        indexes = self._cfg.get_val ("%s!directory_index" % (self._pre), '')
        if indexes.find("index.php") == -1:
            if len(indexes) > 0:
                self._cfg["%s!directory_index" % (self._pre)] = 'index.php,%s' % (indexes)
            else:
                self._cfg["%s!directory_index" % (self._pre)] = 'index.php'


#
# Helpers
#
def wizard_php_get_info (cfg, pre):
    wizard = Wizard_Rules_PHP (cfg, pre)

    misses = wizard.show()
    if misses:
        return None

    return {'source': wizard.source,
            'rule':   wizard.rule,
            'nick':   wizard.nick}

def wizard_php_get_source_info (cfg):
    wizard = Wizard_Rules_PHP (cfg, None)
    wizard.show()

    if not wizard.source:
        return None

    return {'source': wizard.source,
            'nick':   wizard.nick}
    
