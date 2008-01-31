import re

from Entry import *
from Module import *

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
        entry = Entry (cfg_key, 'text', self._cfg)
        tup = (title, entry)
        if extra_cols:
            tup += extra_cols

        table += tup

    def AddTableEntryRemove (self, table, title, cfg_key):
        form = Form ("/%s/update" % (self._id), submit_label="Del")
        button = form.Render ('<input type="hidden" name="%s" value="" />' % (cfg_key))
        self.AddTableEntry (table, title, cfg_key, (button,))

    def AddTableOptions (self, table, title, cfg_key, options, *args, **kwargs):
        try:
            value = self._cfg[cfg_key].value
            ops = EntryOptions (cfg_key, options, selected=value, *args, **kwargs)
        except AttributeError:
            value = ''
            ops = EntryOptions (cfg_key, options, *args, **kwargs)

        table += (title, ops)
        return value

    def AddTableOptions_w_Properties (self, table, title, cfg_key, options, 
                                      props, props_prefix="prop_"):
        # The Table entry itself
        value = self.AddTableOptions (table, title, cfg_key, options, id="%s" % (cfg_key), 
                                      onChange="options_active_prop('%s','%s');" % (cfg_key, props_prefix))
        # The entries that come after
        props_txt  = ''
        for name, desc in options:
            if not name:
                continue
            props_txt  += '<div id="%s%s">%s</div>\n' % (props_prefix, name, props[name])

        # Show active property
        update = '<script type="text/javascript">\n' + \
                 '   options_active_prop("%s","%s");\n' % (cfg_key, props_prefix) + \
                 '</script>\n';
        return props_txt + update

    def AddTableOptions_w_ModuleProperties (self, table, title, cfg_key, options):
        props = {}
        for name, desc in options:
            if not name:
                continue
            try:
                props_widget = module_obj_factory (name, self._cfg, cfg_key)
                render = props_widget._op_render()
            except IOError:
                render = "Couldn't load the properties module: %s" % (name)
            props[name] = render

        return self.AddTableOptions_w_Properties (table, title, cfg_key, options, props)

    def AddTableCheckbox (self, table, title, cfg_key, default=None):
        try:
            tmp = self._cfg[cfg_key].value.lower()
            if tmp in ["on", "1", "true"]: 
                value = '1'
            else:
                value = '0'
        except:
            value = None

        if value == '1':
            entry = Entry (cfg_key, 'checkbox', checked=value)
        elif value == '0':
            entry = Entry (cfg_key, 'checkbox')
        else:
            if default == True:
                entry = Entry (cfg_key, 'checkbox', checked='1')
            elif default == False:
                entry = Entry (cfg_key, 'checkbox')
            else:
                entry = Entry (cfg_key, 'checkbox')

        table += (title, entry)

    # Applying changes
    #

    def _ValidateChanges (self, post, validation):
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

    def ApplyChanges (self, checkboxes, post, validation=None):
        # Validate changes
        if validation:
            self._ValidateChanges (post, validation)
        
        # Apply checkboxes
        for key in checkboxes:
            if key in post:
                self._cfg[key] = post[key][0]
            else:
                self._cfg[key] = "0"

        # Apply text entries
        for confkey in post:
            if not confkey in checkboxes:
                value = post[confkey][0]
                if not value:
                    del (self._cfg[confkey])
                else:
                    self._cfg[confkey] = value
        
    def ApplyChangesPrefix (self, prefix, checkboxes, post, validation=None):
        checkboxes_pre = map(lambda x, p=prefix: "%s!%s"%(p,x), checkboxes)
        return self.ApplyChanges (checkboxes_pre, post, validation)
