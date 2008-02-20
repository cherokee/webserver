import re

from Entry import *
from Module import *

FORM_TEMPLATE = """
<form action="%(action)s" method="%(method)s">
  %(content)s
  %(submit)s
</form>
"""

SUBMIT_BUTTON = """
<input type="submit" %(submit_props)s />
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
        keys = {'submit':       '',
                'submit_props': '',
                'content':      content,
                'action':       self._action,
                'method':       self._method}
                
        if self._add_submit:
            keys['submit'] = SUBMIT_BUTTON
        if self._submit_label:
            keys['submit_props'] = 'value="%s" '%(self._submit_label)

        render = FORM_TEMPLATE
        while '%(' in render:
            render = render % keys
        return render

    
class FormHelper (WebComponent):
    def __init__ (self, id, cfg):
        WebComponent.__init__ (self, id, cfg)
        self.errors = {}
    
    def Indent (self, content):
        return '<div class="indented">%s</div>' %(content)
        
    def Dialog (self, txt, tipe='information'):
        return '<div class="dialog-%s">%s</div>' % (tipe, txt)

    def HiddenInput (self, key, value):
        return '<input type="hidden" value="%s" id="%s" name="%s" />' % (value, key, key)

    def InstanceEntry (self, cfg_key, tipe, **kwargs):
        # Instance an Entry
        entry = Entry (cfg_key, tipe, self._cfg, **kwargs)
        txt = str(entry)

        # Check whether there is a related error
        if cfg_key in self.errors.keys():
            msg, val = self.errors[cfg_key]
            txt += '<div class="error"><b>%s</b>: %s</div>' % (val, msg)

        return txt

    def Label(self, title, for_id):
        txt = '<label for="%s">%s</label>' % (for_id, title)
        return txt

    def InstanceTab (self, entries):
        # HTML
        txt = '<dl class="tab" id="tab_%s">' % (self._id)
        for title, content in entries:
            txt += '<dt>%s</dt>\n' % (title)
            txt += '<dd>%s</dd>\n' % (content)
        txt += '</dl>'

        # Javascript
        txt += '<script type="text/javascript">'
        txt += 'jQuery("#tab_%s").Accordion({' % (self._id)
        txt += '  autoheight: true,';
        txt += '  animated: "easeslide",';
        txt += '  alwaysOpen: false';
        txt += '});' 
        txt += '</script>'

        return txt

    def AddTableEntry (self, table, title, cfg_key, extra_cols=None):
        # Get the entry
        txt = self.InstanceEntry (cfg_key, 'text')

        # Add to the table
        label = self.Label(title, cfg_key);
        tup = (label, txt)
        if extra_cols:
            tup += extra_cols
        table += tup

    def InstanceButton (self, name, **kwargs):
        extra = ""
        for karg in kwargs:
            extra += '%s="%s" '%(karg, kwargs[karg])

        return '<input type="button" value="%s" %s/>' % (name, extra)

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

        label = self.Label(title, cfg_key);
        table += (label, ops)
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
            props_txt += '<div id="%s%s">%s</div>\n' % (props_prefix, name, props[name])

        # Show active property
        update = '<script type="text/javascript">\n' + \
                 '   options_active_prop("%s","%s");\n' % (cfg_key, props_prefix) + \
                 '</script>\n';
        return props_txt + update

    def AddTableOptions_w_ModuleProperties (self, table, title, cfg_key, options, **kwargs):
        props = {}
        for name, desc in options:
            if not name:
                continue
            try:
                props_widget = module_obj_factory (name, self._cfg, cfg_key, **kwargs)
                render = props_widget._op_render()
            except IOError:
                render = "Couldn't load the properties module: %s" % (name)
            props[name] = render

        return self.AddTableOptions_w_Properties (table, title, cfg_key, options, props)

    def InstanceCheckbox (self, cfg_key, default=None):
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
        return entry

    def AddTableCheckbox (self, table, title, cfg_key, default=None):
        entry = self.InstanceCheckbox (cfg_key, default)
        label = self.Label(title, cfg_key);
        table += (label, entry)

    # Errors
    #

    def _error_add (self, key, wrong_val, msg):
        self.errors[key] = (msg, str(wrong_val))
        
    def has_errors (self):
        return len(self.errors) > 0

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
                    try:
                        tmp = validation_func (value)
                        post[post_entry] = [tmp]
                    except ValueError, error:
                        self._error_add (post_entry, value, error)

    def ApplyChanges (self, checkboxes, post, validation=None):
        # Validate changes
        if validation:
            self._ValidateChanges (post, validation)
        
        # Apply checkboxes
        for key in checkboxes:
            if key in self.errors:
                continue
            if key in post:
                self._cfg[key] = post[key][0]
            else:
                self._cfg[key] = "0"

        # Apply text entries
        for confkey in post:
            if not '!' in confkey:
                continue

            if confkey in self.errors:
                continue
            if not confkey in checkboxes:
                value = post[confkey][0]
                if not value:
                    del (self._cfg[confkey])
                else:
                    self._cfg[confkey] = value
        
    def ApplyChangesPrefix (self, prefix, checkboxes, post, validation=None):
        checkboxes_pre = map(lambda x, p=prefix: "%s!%s"%(p,x), checkboxes)
        return self.ApplyChanges (checkboxes_pre, post, validation)

    def CleanUp_conf_props (self, cfg_key, name):
        module  = module_obj_factory (name, self._cfg, cfg_key)
        props   = module.__class__.PROPERTIES

        to_be_deleted = []
        for entry in self._cfg[cfg_key]:
            prop_cfg = "%s!%s" % (cfg_key, entry)
            if entry not in props:
                to_be_deleted.append (prop_cfg)

        for entry in to_be_deleted:
            del(self._cfg[entry])


