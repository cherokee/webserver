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
        <div id="help-area"><a id="help" name="help" href="#help" onclick="toggle_help();">Help</a>
     	    <div id="help-contents">%(help)s</div>
        </div>
        %(content)s
        </div></div>
    </div>
    <div class="clearfix"></div>
"""

PAGE_MENU_MENU = """
<ul id="nav">
<li id="general"><a href="/general">General</a></li>
<li id="vserver"><a href="/vserver">Virtual Servers</a></li>
<li id="encoder"><a href="/encoder">Encoding</a></li>
<li id="icon"><a href="/icons">Icons</a></li>
<li id="mime"><a href="/mime">Mime Types</a></li>
<li id="advanced"><a href="/advanced">Advanced</a></li>
</ul>

<br />

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

class Page (WebComponent):
    def __init__ (self, id, cfg=None):
        WebComponent.__init__ (self, id, cfg)

        self.macros  = {}
        self.theme   = Theme('default')

        self.AddMacroContent ('current', id)
        self.AddMacroContent ('version', VERSION)

    def Render (self):
        return self.theme.BuildTemplate (self.macros, self._id)

    def AddMacroContent (self, key, value):
        self.macros[key] = value

    def Read (self, name):
        return self.theme.ReadFile(name)


class PageMenu (Page):
    def __init__ (self, id, cfg):
        Page.__init__ (self, id, cfg)

        manager = cherokee_management_get (cfg)
        if manager.is_alive():
            self.AddMacroContent ('menu_save_dropdown', MENU_SAVE_IS_ALIVE)
            self.AddMacroContent ('menu_save_desc', 'Commit to disk and apply changes to the running server')
        else:
            self.AddMacroContent ('menu_save_dropdown', '')
            self.AddMacroContent ('menu_save_desc', 'Commit all the changes permanently')

        self.AddMacroContent ('body', PAGE_MENU_LAYOUT)
        self.AddMacroContent ('menu', PAGE_MENU_MENU)
