from config import *
from util import *
from Page import *
from Wizard import *
from Wizard_PHP import wizard_php_get_info
from Wizard_PHP import wizard_php_get_source_info

NOTE_SOURCES = _("Path to the directory where the Wordpress source code is located. (Example: /usr/share/wordpress)")
NOTE_WEB_DIR = _("Web directory where you want Wordpress to be accessible. (Example: /blog)")
NOTE_HOST    = _("Host name of the virtual host that is about to be created.")
ERROR_NO_SRC = _("Does not look like a WordPress source directory.")

CONFIG_DIR = """
%(pre_rule_0)s!document_root = %(local_src_dir)s
%(pre_rule_0)s!match = directory
%(pre_rule_0)s!match!directory = %(web_dir)s
%(pre_rule_0)s!match!final = 0

%(pre_rule_1)s!match = and
%(pre_rule_1)s!match!final = 1
%(pre_rule_1)s!match!left = directory
%(pre_rule_1)s!match!left!directory = %(web_dir)s
%(pre_rule_1)s!match!right = exists
%(pre_rule_1)s!match!right!iocache = 1
%(pre_rule_1)s!match!right!match_any = 1
%(pre_rule_1)s!handler = file
%(pre_rule_1)s!handler!iocache = 1
"""

CONFIG_VSRV = """
%(pre_vsrv)s!document_root = %(local_src_dir)s
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!directory_index = index.php,index.html

%(pre_rule_0)s!match = request
%(pre_rule_0)s!match!request = ^/$
%(pre_rule_0)s!handler = redir
%(pre_rule_0)s!handler!rewrite!1!substring = /index.php
%(pre_rule_0)s!handler!rewrite!1!show = 0

%(pre_rule_1)s!match = exists
%(pre_rule_1)s!match!iocache = 1
%(pre_rule_1)s!match!match_any = 1
%(pre_rule_1)s!match!match_only_files = 1
%(pre_rule_1)s!handler = file
%(pre_rule_1)s!handler!iocache = 1

%(pre_vsrv)s!rule!1!match = default
%(pre_vsrv)s!rule!1!handler = common
"""

class Page1 (PageMenu, FormHelper):
    def __init__ (self, cfg, pre, uri, type):
        PageMenu.__init__ (self, "WordPress_Page1", cfg, [])
        FormHelper.__init__ (self, "WordPress_Page1", cfg)
        self._pre    = pre
        self.url_pre = uri
        self.type    = type
        self.title = _('Wordpress Wizard: Location')

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', _('Worpress Wizard: Page 1'))
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_content (self):
        txt = '<h1>%s</h1>' % (self.title)
        
        table = TableProps()
        self.AddPropEntry (table, _('Source Directory'),  'tmp!wizard_wp!sources', NOTE_SOURCES)
        if self.type == "dir":
            self.AddPropEntry (table, _('Web Directory'), 'tmp!wizard_wp!web_dir', NOTE_WEB_DIR, value="/blog")
        else:
            self.AddPropEntry (table, _('New Host Name'), 'tmp!wizard_wp!host', NOTE_HOST, value="blog.example.com")
        txt += self.Indent(table)

        form = Form (self.url_pre, add_submit=True, auto=False)
        return form.Render(txt, DEFAULT_SUBMIT_VALUE)

        return txt

    def __apply_cfg_chunk (self, config_txt):
        lines = config_txt.split('\n')
        lines = filter (lambda x: len(x) and x[0] != '#', lines)

        for line in lines:
            line = line.strip()
            left, right = line.split (" = ", 2)
            self._cfg[left] = right

    def _op_apply_dir (self, post):
        tmp = self.url_pre.split("/")
        vserver       = tmp[2]

        # Incoming info
        local_src_dir = post.pop('tmp!wizard_wp!sources')
        web_dir       = post.pop('tmp!wizard_wp!web_dir')

        if not local_src_dir or not web_dir:
            return 

        # Validation
        if not os.path.exists (os.path.join (local_src_dir, "wp-login.php")):
            self.errors['tmp!wizard_wp!sources'] = (ERROR_NO_SRC, local_src_dir)
            return

        # Replacement
        php_info = wizard_php_get_info (self._cfg, self._pre)
        php_rule = int (php_info['rule'].split('!')[-1])

        pre_rule_0 = "vserver!%s!rule!%d" % (vserver, php_rule - 1)
        pre_rule_1 = "vserver!%s!rule!%d" % (vserver, php_rule - 2)

        # Add the new rules
        config = CONFIG_DIR % (locals())
        self.__apply_cfg_chunk (config)

    def _op_apply_vserver (self, post):
        pre_vsrv = cfg_vsrv_get_next (self._cfg)
        vsrv_id  = pre_vsrv.split('!')[-1]

        # Incoming info
        local_src_dir = post.pop('tmp!wizard_wp!sources')
        host          = post.pop('tmp!wizard_wp!host')

        # Validation
        if not os.path.exists (os.path.join (local_src_dir, "wp-login.php")):
            self.errors['tmp!wizard_wp!sources'] = (ERROR_NO_SRC, local_src_dir)
            return
        
        # Add PHP Rule
        from Wizard_PHP import Wizard_Rules_PHP
        php_wizard = Wizard_Rules_PHP (self._cfg, pre_vsrv)
        php_wizard.show()
        php_wizard.run (pre_vsrv, None)

        # Replacement
        php_info = wizard_php_get_info (self._cfg, pre_vsrv)
        php_rule = int (php_info['rule'].split('!')[-1])

        pre_rule_0 = "%s!rule!%d" % (pre_vsrv, php_rule - 1)
        pre_rule_1 = "%s!rule!%d" % (pre_vsrv, php_rule - 2)

        # Add the new rules    
        config = CONFIG_VSRV % (locals())
        self.__apply_cfg_chunk (config)



class Wizard_Rules_WordPress (Wizard):
    ICON = "wordpress.jpg"
    DESC = "Configures Wordpress inside a public web directory."

    def __init__ (self, cfg, pre):
        Wizard.__init__ (self, cfg, pre)
        self.name  = "Configure Wordpress"
        self.group = WIZARD_GROUP_CMS

    def show (self):
        # Check for PHP
        php_info = wizard_php_get_info (self._cfg, self._pre)
        if not php_info:
            self.no_show = "PHP support is required."
            return False

        return True

    def _run (self, uri, post):
        page = Page1 (self._cfg, self._pre, uri, "dir")
        if post.get_val('is_submit'):
            re = page._op_apply_dir (post)
            if page.has_errors():
                return page._op_render()
        else:
            return page._op_render()


class Wizard_VServer_WordPress (Wizard):
    ICON = "wordpress.jpg"
    DESC = "Configure a new virtual server using Wordpress."

    def __init__ (self, cfg, pre):
        Wizard.__init__ (self, cfg, pre)
        self.name  = "Add a new Wordpress virtual server"
        self.group = WIZARD_GROUP_CMS

    def show (self):
        # Check for a PHP Information Source
        php_source = wizard_php_get_source_info (self._cfg)
        if not php_source:
            self.no_show = "PHP support is required."
            return False
        return True

    def _run (self, uri, post):
        page = Page1 (self._cfg, self._pre, uri, "vserver")
        if post.get_val('is_submit'):
            re = page._op_apply_vserver (post)
            if page.has_errors():
                return page._op_render()
        else:
            return page._op_render()
