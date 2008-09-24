from Form import *
from Theme import *
from Entry import *
from configured import *
from CherokeeManagement import *
        
PAGE_BASIC_LAYOUT = """
    <div id="container">
        <div id="bar">
	   <div id="logo"><a href="/"><img src="/static/images/cherokee-logo-bar.png" alt="logo"/></a></div>
	   <div id="version">Version: %(version)s</div>
        </div>
        <div id="workarea"><div id="workarea-inner">
        %(content)s
        </div></div>
    </div>
    <div class="clearfix"></div>
"""

PAGE_MENU_LAYOUT = """
    <div id="container">
        <div id="bar">
	   <div id="logo"><a href="/"><img src="/static/images/cherokee-logo-bar.png" alt="logo"/></a></div>
	   <div id="version">Version: %(version)s</div>
            %(menu)s
        </div>
        <div id="workarea"><div id="workarea-inner">
        %(content)s
        </div></div>
    </div>
    <div class="clearfix"></div>
"""

PAGE_MENU_HELP_BLOCK = """
<h2>Help</h2>
%(help)s
<br />
"""

PAGE_MENU_MENU = """
<ul id="nav">
<li id="status"><a href="/">Status</a></li>
<li id="general"><a href="/general">General</a></li>
<li id="vserver"><a href="/vserver">Virtual Servers</a></li>
<li id="source"><a href="/source">Info Sources</a></li>
<li id="icon"><a href="/icons">Icons</a></li>
<li id="mime"><a href="/mime">Mime Types</a></li>
<li id="advanced"><a href="/advanced">Advanced</a></li>
</ul>
<br />

%(help_block)s

<h2>Save Changes</h2>
<div style="padding-top: 2px;">
 <p>%(menu_save_desc)s</p>
</div>

<form id="form-apply" action="/apply" method="post">
 %(menu_save_dropdown)s

 <div style="float: center; padding-top: 4px;">
  <a class="button" href="#" onclick="this.blur(); $('#form-apply').submit(); return false;"><span>Save</span></a>
 </div>
</form>

"""

MENU_SAVE_IS_ALIVE = """
  <div style="padding-top: 2px;">
    <select name="restart">
      <option value="graceful">Graceful restart</option>
      <option value="hard">Hard restart</option>
      <option value="no">Do not restart</option>
    </select>
  </div>
"""

DIALOG_W=725
DIALOG_H=500

class Page (WebComponent):
    def __init__ (self, id, cfg=None, helps=[]):
        WebComponent.__init__ (self, id, cfg)

        self.macros  = {}
        self.helps   = helps[:]
        self.theme   = Theme('default')

        self.AddMacroContent ('current', id)
        self.AddMacroContent ('version', VERSION)

    def Render (self):
        self._add_help_macro()
        return self.theme.BuildTemplate (self.macros, self._id)

    def AddMacroContent (self, key, value):
        self.macros[key] = value

    def Read (self, name):
        return self.theme.ReadFile(name)

    # Help
    #
    def AddHelp (self, (help_file, comment)):
        for h,c in self.helps:
            if h == help_file:
                return
        self.helps.append ((help_file, comment))

    def AddHelps (self, helps):
        for h in helps:
            self.AddHelp(h)

    def _render_help (self):
        if not self.helps:
            return ''

        txt = '<ul>'
        for hfile,comment in self.helps:
            if hfile.startswith("http://"):
                txt += '<li><a href="%s" target="_help">%s</a></li>' % (hfile, comment)
            else:
                params=(hfile, DIALOG_H, DIALOG_W, comment)
                txt += '<li><a href="/help/%s.html?KeepThis=true&TB_iframe=true&height=%s&width=%s" class="thickbox">%s</a></li>' % params
        txt += '</ul>'
        return txt

    def _add_help_macro (self):
        help_txt = self._render_help()
        if help_txt:
            self.AddMacroContent ('help_block', PAGE_MENU_HELP_BLOCK)
        else:
            self.AddMacroContent ('help_block', '')
        self.AddMacroContent ('help', help_txt)


class PageMenu (Page):
    def __init__ (self, id, cfg, helps=[]):
        Page.__init__ (self, id, cfg, helps)

        manager = cherokee_management_get (cfg)
        if manager.is_alive():
            self.AddMacroContent ('menu_save_dropdown', MENU_SAVE_IS_ALIVE)
            self.AddMacroContent ('menu_save_desc', 'Commit to disk and apply changes to the running server')
        else:
            self.AddMacroContent ('menu_save_dropdown', '')
            self.AddMacroContent ('menu_save_desc', 'Commit all the changes permanently')

        self.AddMacroContent ('body', PAGE_MENU_LAYOUT)
        self.AddMacroContent ('menu', PAGE_MENU_MENU)
