import validations

from config import *
from util import *
from Page import *
from Wizard import *

# For gettext
N_ = lambda x: x

ROR_CHILD_PROCS = 3
DEFAULT_BINS    = ['spawn-fcgi']

NOTE_ROR_DIR    = N_("Local path to the Ruby on Rails based project.")
NOTE_NEW_HOST   = N_("Name of the new domain that will be created.")
NOTE_NEW_DIR    = N_("Directory of the web directory where the Ruby on Rails project will live in.")
NOTE_ENV        = N_("Value of the RAILS_ENV variable.")
NOTE_METHOD     = N_("It is recommended to proxy the best available option, but FastCGI can also be used.")

ERROR_DISPATCH  = N_("<p>Even though the directory looks like a Ruby on Rails project, the public/dispatch.fcgi file wasn't found.</p>")
ERROR_EXAMPLE   = N_("<p>However a <b>public/dispatch.fcgi.example</b> file is present, so you might want to rename it.</p>")
ERROR_RAILS23   = N_("<p>If you are using Rails >= 2.3.0, you will have to execute the following command from the project directory in order to add the missing file:</p><p><pre>rake rails:update:generate_dispatchers</pre></p>")
ERROR_NO_ROR    = N_("It does not look like a Ruby on Rails based project directory.")
ERROR_NO_DROOT  = N_("The document root directory does not exist.")

RAILS_ENV = [
    ('production',  'Production'),
    ('test',        'Test'),
    ('development', 'Development'),
    ('',            'Empty')
]

RAILS_METHOD = [
    ('proxy',  'HTTP proxy'),
    ('fcgi',   'FastCGI')
]

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = RoR %(new_host)s, instance %(src_instance)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
"""

SOURCE_FCGI = """
source!%(src_num)d!interpreter = spawn-fcgi -n -d %(ror_dir)s -f %(ror_dir)s/public/dispatch.fcgi -p %(src_port)d
"""

SOURCE_PROXY = """
source!%(src_num)d!interpreter = %(ror_dir)s/script/server -p %(src_port)d
"""

SOURCE_ENV = """
source!%(src_num)d!env!RAILS_ENV = %(ror_env)s
"""

CONFIG_VSRV = """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(ror_dir)s/public
%(vsrv_pre)s!directory_index = index.html

%(vsrv_pre)s!rule!10!match = exists
%(vsrv_pre)s!rule!10!match!match_any = 1
%(vsrv_pre)s!rule!10!match!match_only_files = 1
%(vsrv_pre)s!rule!10!match!match_index_files = 1
%(vsrv_pre)s!rule!10!handler = common
%(vsrv_pre)s!rule!10!expiration = time
%(vsrv_pre)s!rule!10!expiration!time = 7d

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
"""

CONFIG_VSRV_FCGI = """
%(vsrv_pre)s!rule!1!handler = fcgi
%(vsrv_pre)s!rule!1!handler!error_handler = 1
%(vsrv_pre)s!rule!1!handler!check_file = 0
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
"""

CONFIG_VSRV_PROXY = """
%(vsrv_pre)s!rule!1!handler = proxy
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!in_allow_keepalive = 1
"""

CONFIG_VSRV_CHILD = """
%(vsrv_pre)s!rule!1!handler!balancer!source!%(src_instance)d = %(src_num)d
"""

CONFIG_RULES = """
%(rule_pre_plus2)s!match = directory
%(rule_pre_plus2)s!match!directory = %(webdir)s
%(rule_pre_plus2)s!match!final = 0
%(rule_pre_plus2)s!document_root = %(ror_dir)s/public

%(rule_pre_plus1)s!match = and
%(rule_pre_plus1)s!match!left = directory
%(rule_pre_plus1)s!match!left!directory = %(webdir)s
%(rule_pre_plus1)s!match!right = exists
%(rule_pre_plus1)s!match!right!match_any = 1
%(rule_pre_plus1)s!match!right!any_file = 1
%(rule_pre_plus1)s!match!right!match_only_files = 0
%(rule_pre_plus1)s!handler = common
%(rule_pre_plus1)s!expiration = time
%(rule_pre_plus1)s!expiration!time = 7d

%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
"""

CONFIG_RULES_FCGI = """
%(rule_pre)s!handler = fcgi
%(rule_pre)s!handler!error_handler = 1
%(rule_pre)s!handler!check_file = 0
%(rule_pre)s!handler!balancer = round_robin
"""

CONFIG_RULES_PROXY = """
%(rule_pre)s!handler = proxy
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!in_allow_keepalive = 1
"""

CONFIG_RULES_CHILD = """
%(rule_pre)s!handler!balancer!source!%(src_instance)d = %(src_num)d
"""

def is_ror_dir (path, cfg, nochroot):
    path = validations.is_local_dir_exists (path, cfg, nochroot)
    manage = os.path.join (path, "script/server")
    if not os.path.exists (manage):
        raise ValueError, _(ERROR_NO_ROR)
    return path

DATA_VALIDATION = [
    ("tmp!wizard_ror!ror_dir",  (is_ror_dir, 'cfg')),
    ("tmp!wizard_ror!new_host", (validations.is_new_host, 'cfg'))
]

class CommonMethods:
    def show (self):
        spawn_fcgi = path_find_binary (DEFAULT_BINS)
        if not spawn_fcgi:
            self.no_show = _("Could not find the spawn-fcgi binary")
            return False
        return True

    def _render_content_dispatch_fcgi (self):
        if self.errors.has_key('dispatch.fcgi'):
            if self.errors.has_key('dispatch.fcgi.example'):
                return self.Indent(self.Dialog(_(ERROR_DISPATCH) + _(ERROR_EXAMPLE), 'important-information'))
            else:
                return self.Indent(self.Dialog(_(ERROR_DISPATCH) + _(ERROR_RAILS23), 'important-information'))
        return ''

    def _op_apply_dispatch_fcgi (self, post):
        # Incoming info
        ror_dir  = post.get_val('tmp!wizard_ror!ror_dir')
        new_host = post.get_val('tmp!wizard_ror!new_host')

        # Check whether dispatch.fcgi is present
        if not os.path.exists (os.path.join (ror_dir, "public/dispatch.fcgi")):
            self._cfg['tmp!wizard_ror!ror_dir']  = ror_dir
            self._cfg['tmp!wizard_ror!new_host'] = new_host

            self.errors['dispatch.fcgi'] = 'Not found'
            if os.path.exists (os.path.join (ror_dir, "public/dispatch.fcgi.example")):
                self.errors['dispatch.fcgi.example'] = '1'
            return True

        del(self._cfg['tmp!wizard_ror!ror_dir'])
        del(self._cfg['tmp!wizard_ror!new_host'])



class Wizard_VServer_RoR (CommonMethods, WizardPage):
    ICON = "ror.png"
    DESC = _("New virtual server based on a Ruby on Rails project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/wizard/RoR',
                             id     = "RoR_Page1",
                             title  = _("Ruby on Rails Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>%s</h2>' % (_("New Virtual Server"))
        table = TableProps()
        self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_ror!new_host',      _(NOTE_NEW_HOST), value="www.example.com")
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Ruby on Rails Project"))
        txt += self._render_content_dispatch_fcgi()

        # Trim deployment options if needed
        if not path_find_binary (DEFAULT_BINS):
            RAILS_METHOD.remove(('fcgi', 'FastCGI'))

        table = TableProps()
        self.AddPropEntry   (table, _('Project Directory'),     'tmp!wizard_ror!ror_dir', _(NOTE_ROR_DIR))
        self.AddPropOptions (table, _('RAILS_ENV environment'), 'tmp!wizard_ror!ror_env', RAILS_ENV, _(NOTE_ENV))
        if len(RAILS_METHOD) > 1:
            self.AddPropOptions (table, _('Deployment method'), 'tmp!wizard_ror!ror_method', RAILS_METHOD, _(NOTE_METHOD))
        txt += self.Indent(table)

        if not len(RAILS_METHOD) > 1:
            txt += self.HiddenInput ('tmp!wizard_ror!ror_method', RAILS_METHOD[0][0])

        txt += '<h2>%s</h2>' % (_("Logging"))
        txt += self._common_add_logging()

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        ror_dir    = post.pop('tmp!wizard_ror!ror_dir')
        new_host   = post.pop('tmp!wizard_ror!new_host')
        ror_env    = post.pop('tmp!wizard_ror!ror_env')
        ror_method = post.pop('tmp!wizard_ror!ror_method')

        # Locals
        vsrv_pre = cfg_vsrv_get_next (self._cfg)
        src_num, src_pre = cfg_source_get_next (self._cfg)

        # Usual Static files
        self._common_add_usual_static_files ("%s!rule!500" % (vsrv_pre))

        # Deployment method distinction
        CONFIG = CONFIG_VSRV
        SRC    = SOURCE
        if ror_method == 'fcgi':
            CONFIG += CONFIG_VSRV_FCGI
            SRC    += SOURCE_FCGI

            # Check whether dispatch.fcgi is present
            if post.get_val('tmp!wizard_ror!ror_dir'):
                error = self._op_apply_dispatch_fcgi (post)
                if error:
                    return
        else:
            CONFIG += CONFIG_VSRV_PROXY
            SRC    += SOURCE_PROXY

        # Add the new main rules
        config = CONFIG % (locals())

        # Add the Information Sources
        free_port = cfg_source_find_free_port()
        for i in range(ROR_CHILD_PROCS):
            src_instance = i + 1
            src_port     = i + free_port
            config += SRC % (locals())
            if ror_env:
                config += SOURCE_ENV % (locals())
            config += CONFIG_VSRV_CHILD % (locals())
            src_num += 1

        self._apply_cfg_chunk (config)
        self._common_apply_logging (post, vsrv_pre)

class Wizard_Rules_RoR (CommonMethods, WizardPage):
    ICON = "ror.png"
    DESC = _("New directory based on a Ruby of Rails project.")

    def __init__ (self, cfg, pre):
        WizardPage.__init__ (self, cfg, pre,
                             submit = '/vserver/%s/wizard/RoR'%(pre.split('!')[1]),
                             id     = "RoR_Page1",
                             title  = _("Ruby on Rails Wizard"),
                             group  = _(WIZARD_GROUP_PLATFORM))

    def _render_content (self, url_pre):
        txt = '<h1>%s</h1>' % (self.title)

        txt += '<h2>%s</h2>' % (_("Web Directory"))
        table = TableProps()
        self.AddPropEntry (table, _('Web Directory'), 'tmp!wizard_ror!new_webdir', _(NOTE_NEW_DIR), value="/project")
        txt += self.Indent(table)

        txt += '<h2>%s</h2>' % (_("Ruby on Rails Project"))
        txt += self._render_content_dispatch_fcgi()

        # Trim deployment options if needed
        if not path_find_binary (DEFAULT_BINS):
            RAILS_METHOD.remove(('fcgi', 'FastCGI'))

        table = TableProps()
        self.AddPropEntry   (table, _('Project Directory'),     'tmp!wizard_ror!ror_dir', _(NOTE_ROR_DIR))
        self.AddPropOptions (table, _('RAILS_ENV environment'), 'tmp!wizard_ror!ror_env', RAILS_ENV, NOTE_ENV)
        if len(RAILS_METHOD) > 1:
            self.AddPropOptions (table, _('Deployment method'), 'tmp!wizard_ror!ror_method', RAILS_METHOD, _(NOTE_METHOD))
        txt += self.Indent(table)

        if not len(RAILS_METHOD) > 1:
            txt += self.HiddenInput ('tmp!wizard_ror!ror_method', RAILS_METHOD[0][0])

        form = Form (url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)
        return txt

    def _op_apply (self, post):
        # Store tmp, validate and clean up tmp
        self._cfg_store_post (post)

        self._ValidateChanges (post, DATA_VALIDATION)
        if self.has_errors():
            return

        self._cfg_clean_values (post)

        # Incoming info
        ror_dir    = post.pop('tmp!wizard_ror!ror_dir')
        webdir     = post.pop('tmp!wizard_ror!new_webdir')
        ror_env    = post.pop('tmp!wizard_ror!ror_env')
        ror_method = post.pop('tmp!wizard_ror!ror_method')

        # Locals
        rule_num, rule_pre = cfg_vsrv_rule_get_next (self._cfg, self._pre)
        src_num,  src_pre  = cfg_source_get_next (self._cfg)
        new_host           = self._cfg.get_val ("%s!nick"%(self._pre))
        rule_pre_plus2     = "%s!rule!%d" % (self._pre, rule_num + 2)
        rule_pre_plus1     = "%s!rule!%d" % (self._pre, rule_num + 1)

        # Deployment method distinction
        CONFIG = CONFIG_RULES
        SRC    = SOURCE
        if ror_method == 'fcgi':
            CONFIG += CONFIG_RULES_FCGI
            SRC    += SOURCE_FCGI

            # Check whether dispatch.fcgi is present
            if post.get_val('tmp!wizard_ror!ror_dir'):
                error = self._op_apply_dispatch_fcgi (post)
                if error:
                    return
        else:
            CONFIG += CONFIG_RULES_PROXY
            SRC    += SOURCE_PROXY

        # Add the new rules
        config = CONFIG % (locals())

        # Add the Information Sources
        free_port = cfg_source_find_free_port()
        for i in range(ROR_CHILD_PROCS):
            src_instance = i + 1
            src_port     = i + free_port
            config += SRC % (locals())
            if ror_env:
                config += SOURCE_ENV % (locals())
            config += CONFIG_RULES_CHILD % (locals())
            src_num += 1

        self._apply_cfg_chunk (config)
