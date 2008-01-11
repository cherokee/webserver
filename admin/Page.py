from Form import *
from Theme import *
from Entry import *
        
PAGE_MENU_LAYOUT = """
<table border="1">
 <tr><td>
  <table border="1">
   <tr>
    <td>%%menu%%</td>
    <td>
     %%content%%
    </td>
   </tr>
  </table>
 </td></tr>
 <tr><td>
  %%help%%
 </td></tr>
</table>
"""

PAGE_MENU_MENU = """
<ul>
 <li><a href="/general">General</a></li>
 <li><a href="/vserver">Virtual Servers</a></li>
 <li><a href="/icon">Icons</a></li>
 <li><a href="/mime">Mime Types</a></li>
 <li><a href="/advanced">Advanced</a></li>
</ul>

<form action="/apply" method="post">
 <input type="submit" value="Apply Changes" />
</form>

"""


class Page (WebComponent):
    def __init__ (self, id, cfg=None):
        WebComponent.__init__ (self, id, cfg)

        self.theme  = Theme('default')
        self.macros = []

    def Render (self):
        return self.theme.BuildTemplate (self.macros, self._id)

    def AddMacroContent (self, key, value):
        self.macros.append ((key, value))

    def Read (self, name):
        return self.theme.ReadFile(name)


class PageMenu (Page):
    def __init__ (self, id, cfg):
        Page.__init__ (self, id, cfg)

        self.AddMacroContent ('body', PAGE_MENU_LAYOUT)
        self.AddMacroContent ('menu', PAGE_MENU_MENU)

