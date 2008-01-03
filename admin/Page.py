import re

from Form import *
from Theme import *
from Entry import *

class Page:
    def __init__ (self):
        self._id    = None
        self.theme  = Theme('default')
        self.macros = []

    def Render (self):
        return self.theme.BuildTemplate (self.macros, self._id)

    def AddMacroContent (self, key, value):
        self.macros.append ((key, value))

    def Read (self, name):
        return self.theme.ReadFile(name)

    def Handle (self, uri, post):
        ruri = uri[1+len(self._id):]
        if len(ruri) <= 1:
            return self._op_render()

        return self._op_handler(ruri, post)


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

class PageMenu (Page):
    def __init__ (self, cfg):
        Page.__init__ (self)
        self._cfg = cfg

        self.AddMacroContent ('body', PAGE_MENU_LAYOUT)
        self.AddMacroContent ('menu', PAGE_MENU_MENU)

    def AddTableEntry (self, table, title, cfg_key, extra_cols=None):
        try:
            value = self._cfg[cfg_key].value
            entry = Entry (cfg_key, 'text', value=value)
        except AttributeError:
            entry = Entry (cfg_key, 'text')

        tup = (title, entry)
        if extra_cols:
            tup += extra_cols

        table += tup

    def AddTableEntryRemove (self, table, title, cfg_key):
        form = Form ("/%s/update" % (self._id), submit_label="Del")
        button = form.Render ('<input type="hidden" name="%s" value="" />' % (cfg_key))
        self.AddTableEntry (table, title, cfg_key, (button,))

    def AddTableOptions (self, table, title, cfg_key, options):
        try:
            value = self._cfg[cfg_key].value
            options = EntryOptions (cfg_key, options, selected=value)
        except AttributeError:
            options = EntryOptions (cfg_key, options)
        table += (title, options)

    def AddTableCheckbox (self, table, title, cfg_key):
        value = None
        try:
            tmp = self._cfg[cfg_key].value.lower()
            if tmp in ["on", "1", "true"]: 
                value = "1"
        except:
            pass

        if value:
            entry = Entry (cfg_key, 'checkbox', checked=value)
        else:
            entry = Entry (cfg_key, 'checkbox')

        table += (title, entry)

    def ValidateChanges (self, post, validation):
        for rule in validation:
            regex, validation_func = rule
            p = re.compile (regex)
            for post_entry in post:
                if p.match(post_entry):
                    value = post[post_entry][0]
                    if not value:
                        continue
                    tmp = validation_func (value)
                    post[post_entry] = [tmp]
