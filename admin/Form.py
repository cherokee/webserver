import re

from Entry import *

FORM_TEMPLATE = """
<form action="%%action%%" method="%%method%%">
  %%content%%
  %%submit%%
</form>
"""

SUBMIT_BUTTON = """
<input type="submit" %%submit_props%%/>
"""

class WebComponent:
    def __init__ (self, id, cfg):
        self._id  = id
        self._cfg = cfg

    def _op_handler (self, ruri, post):
        raise "Should have been overridden"

    def _op_render (self):
        raise "Should have been overridden"

    def HandleRequest (self, uri, post):
        parts = uri.split('/')[2:]        
        if parts:
            ruri = '/' + reduce (lambda x,y: '%s/%s'%(x,y), parts)
        else:
            ruri = '/'

        if len(ruri) <= 1:
            return self._op_render()

        return self._op_handler(ruri, post)

class Form:
    def __init__ (self, action, method='post', add_submit=True, submit_label=None):
        self._action       = action
        self._method       = method
        self._add_submit   = add_submit
        self._submit_label = submit_label
        
    def Render (self, content=''):
        txt = FORM_TEMPLATE 
        txt = txt.replace ('%%action%%', self._action)
        txt = txt.replace ('%%method%%', self._method)
        txt = txt.replace ('%%content%%', content)

        if self._add_submit:
            txt = txt.replace ('%%submit%%', SUBMIT_BUTTON)
        else:
            txt = txt.replace ('%%submit%%', '')

        if self._submit_label:
            txt = txt.replace ('%%submit_props%%', 'value="%s" '%(self._submit_label))
        else:
            txt = txt.replace ('%%submit_props%%', '')

        return txt

    
class FormHelper (WebComponent):
    def __init__ (self, id, cfg):
        WebComponent.__init__ (self, id, cfg)

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
