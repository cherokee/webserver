from Form import *
from Theme import *
from Entry import *
from configured import *
        
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
<form id="form-apply" action="/apply" method="post">
 <div style="float: center;">
  <a class="button" href="#" onclick="this.blur(); $('#form-apply').submit(); return false;"><span>Apply</span></a>
 </div>
</form>
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

        self.AddMacroContent ('body', PAGE_MENU_LAYOUT)
        self.AddMacroContent ('menu', PAGE_MENU_MENU)

