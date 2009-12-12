import re

from config import *
from util import *
from Wizard import *

PHP_DEFAULT_TIMEOUT        = '30'
SAFE_PHP_FCGI_MAX_REQUESTS = '490'

FPM_BINS = ['php-fpm', 'php5-fpm']
STD_BINS = ['php-cgi', 'php']

DEFAULT_BINS  = FPM_BINS + STD_BINS

DEFAULT_PATHS = ['/usr/bin',
                 '/opt/php',
                 '/usr/php/bin',
                 '/usr/sfw/bin',
                 '/usr/gnu/bin',
                 '/usr/local/bin',
                 '/opt/local/bin',
                 '/usr/pkg/libexec/cgi-bin']

FPM_ETC_PATHS = ['/etc/php*/fpm/php*fpm.conf',
                 '/usr/local/etc/php*fpm.conf',
                 '/opt/php*/etc/php*fpm.conf',
                 '/opt/local/etc/php*/php*fpm.conf',
                 '/etc/php*/*/php*fpm.conf']

STD_ETC_PATHS = ['/etc/php*/cgi/php.ini',
                 '/usr/local/etc/php.ini',
                 '/opt/php*/etc/php.ini',
                 '/opt/local/etc/php*/php.ini',
                 '/etc/php*/*/php.ini']

# IANA: TCP ports 47809-47999 are unassigned

class Wizard_Rules_PHP (Wizard):
    TCP_PORT      = 47990
    ICON          = "php.jpg"
    DESC          = _("Configures PHP in the current Virtual Server. It will add a new .php extension if not present.")

    def __init__ (self, cfg, pre):
        Wizard.__init__ (self, cfg, pre)
        self.name   = _("Add PHP support")
        self.source = None
        self.group  = _(WIZARD_GROUP_LANGS)

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
        msg = _("Already configured: nick")
        self.no_show = '%s=%s' % (msg, self.nick)
        return False

    def __figure__max_execution_time (self):
        # Figure out the php.ini path
        paths = []
        for p in STD_ETC_PATHS:
            paths.append (p)
            paths.append ("%s-*" %(p))

        phpini_path = path_find_w_default (paths, None)
        if not phpini_path:
            return PHP_DEFAULT_TIMEOUT

        # Read the file
        try:
            content = open(phpini_path, "r").read()
        except IOError:
            return PHP_DEFAULT_TIMEOUT

        # Try to read the max_execution_time
        tmp = re.findall (r"max_execution_time\s*=\s*(\d*)", content)
        if not tmp:
            return PHP_DEFAULT_TIMEOUT

        return tmp[0]

    def __figure__fpm_settings (self):
        #find config file
        paths = []
        for p in FPM_ETC_PATHS:
            paths.append (p)
            paths.append ("%s-*" %(p))

        fpm_conf = path_find_w_default (paths, None)
        if not fpm_conf:
            return None

        try:
            content = open(fpm_conf, "r").read()
        except IOError:
            return None

        tmp = re.findall (r'<value name="listen_address">(.*?)</value>', content)
        if tmp:
            listen_address = tmp[0]

        tmp = re.findall (r'<value name="request_terminate_timeout">(\d*)s*</value>', content)
        if tmp:
            timeout = tmp[0]
        else:
            timeout = PHP_DEFAULT_TIMEOUT

        fpm_settings = { 'fpm_conf' :           fpm_conf,
                         'fpm_listen_address' : listen_address,
                         'fpm_terminate_timeout' : timeout }
        return fpm_settings

    def _add_std_source (self, php_path):
        tcp_addr = cfg_source_get_localhost_addr()
        if not tcp_addr:
            return None

        x, self.source = cfg_source_get_next (self._cfg)
        self._cfg['%s!nick' % (self.source)]        = 'PHP Interpreter'
        self._cfg['%s!type' % (self.source)]        = 'interpreter'
        self._cfg['%s!interpreter' % (self.source)] = '%s -b %s:%d' % (php_path, tcp_addr, self.TCP_PORT)
        self._cfg['%s!host' % (self.source)]        = '%s:%d' % (tcp_addr, self.TCP_PORT)
        self._cfg['%s!env!PHP_FCGI_MAX_REQUESTS' % (self.source)] = SAFE_PHP_FCGI_MAX_REQUESTS
        self._cfg['%s!env!PHP_FCGI_CHILDREN' % (self.source)]     = "5"
        return True

    def _add_fpm_source (self, php_path):
        host = self.fpm_settings['fpm_listen_address']
        if not host:
            return None

        x, self.source = cfg_source_get_next (self._cfg)
        self._cfg['%s!nick' % (self.source)]        = 'PHP Interpreter'
        self._cfg['%s!type' % (self.source)]        = 'interpreter'
        self._cfg['%s!interpreter' % (self.source)] = '%s --fpm-config %s' % (php_path, self.fpm_settings['fpm_conf'])
        self._cfg['%s!host' % (self.source)]        = host
        return True

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
                msg  = _("Looked for the binaries")
                desc = "<p>%s: %s.</p>" % (msg, ", ".join(DEFAULT_BINS))
                return self.report_error (_("Couldn't find a suitable PHP interpreter."), desc)

            php_bin = php_path.split('/')[-1]
            if php_bin in FPM_BINS:
                self.fpm_settings = self.__figure__fpm_settings()
                if not self.fpm_settings:
                    return self.report_error (_("Couldn't determine PHP-fpm settings"))
                ret = self._add_fpm_source(php_path)
            else:
                ret = self._add_std_source(php_path)

            if not ret:
                return self.report_error (_("Couldn't determine correct interpreter settings"))

        # Figure the timeout
        interpreter = self._cfg['%s!interpreter' %(self.source)]
        if "fpm" in interpreter:
            timeout = self.fpm_settings['fpm_terminate_timeout']
        else:
            timeout = self.__figure__max_execution_time ()

        # Add a new Extension PHP rule
        if not self.rule:
            x, self.rule = cfg_vsrv_rule_get_next (self._cfg, self._pre)
            if not self.rule:
                return self.report_error (_("Couldn't add a new rule."))

            src_num = self.source.split('!')[-1]

            self._cfg['%s!match' % (self.rule)]                     = 'extensions'
            self._cfg['%s!match!extensions' % (self.rule)]          = 'php'
            self._cfg['%s!match!final' % (self.rule)]               = '0'
            self._cfg['%s!handler' % (self.rule)]                   = 'fcgi'
            self._cfg['%s!handler!balancer' % (self.rule)]          = 'round_robin'
            self._cfg['%s!handler!balancer!source!1' % (self.rule)] = src_num
            self._cfg['%s!handler!error_handler' % (self.rule)]     = '1'
            self._cfg['%s!encoder!gzip' % (self.rule)]              = '1'
            self._cfg['%s!timeout' % (self.rule)]                   = timeout

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

