FORM_TEMPLATE = """
<form action="%%action%%" method="%%method%%">
  %%content%%
  %%submit%%
</form>
"""

SUBMIT_BUTTON = """
<input type="submit" %%submit_props%%/>
"""

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
