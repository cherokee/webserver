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
        self._id         = id
        self._cfg        = cfg

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
    def __init__ (self, action, method='post', add_submit=True):
        self._action       = action
        self._method       = method
        self._add_submit   = add_submit
        
    def Render (self, content=''):
        keys = {'submit':       '',
                'submit_props': '',
                'content':      content,
                'action':       self._action,
                'method':       self._method}
                
        if self._add_submit:
            keys['submit'] = SUBMIT_BUTTON

        render = FORM_TEMPLATE
        while '%(' in render:
            render = render % keys
        return render


class FormHelper (WebComponent):
    autoops_pre = 1

    def __init__ (self, id, cfg):
        WebComponent.__init__ (self, id, cfg)

        self.errors      = {}
        self.submit_url  = None
        
    def set_submit_url (self, url):
        self.submit_url = url

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

    def AddTableOptions (self, table, title, cfg_key, options, *args, **kwargs):
        # Dirty hack! PoC (1)
        wrap_id = None
        if 'wrap_id' in kwargs:
            wrap_id = kwargs['wrap_id']
            del(kwargs['wrap_id'])

        try:
            value = self._cfg[cfg_key].value
            ops = EntryOptions (cfg_key, options, selected=value, *args, **kwargs)
        except AttributeError:
            value = ''
            ops = EntryOptions (cfg_key, options, *args, **kwargs)

        # Dirty hack! PoC (2)
        if wrap_id:
            ops = '<div id="%s" name="%s">%s</div>'%(wrap_id, wrap_id, ops)

        label = self.Label(title, cfg_key);
        table += (label, ops)
        return value

    def AddTableOptions_Reload (self, table, title, cfg_key, options, **kwargs):
        assert (self.submit_url)

        # Properties prefix
        props_prefix = "auto_options_%d_" % (FormHelper.autoops_pre)
        FormHelper.autoops_pre += 1

        # The Table entry itself
        js = "options_changed('/ajax/update','%s','%s');" % (cfg_key, props_prefix)
        name = self.AddTableOptions (table, title, cfg_key, options, 
                                     onChange=js, wrap_id=props_prefix)
        
        # If there was no cfg value, pick the first
        if not name:
            name = options[0][0]
        
        # Render active option
        if name:
            try:
                # Inherit the errors, if any
                kwargs['errors'] = self.errors
                
                props_widget = module_obj_factory (name, self._cfg, cfg_key, 
                                                   self.submit_url, **kwargs)
                render = props_widget._op_render()
            except IOError:
                render = "Couldn't load the properties module: %s" % (name)
        else:
            render = ''
        
        return render

    def AddTableOptions_w_Properties (self, table, title, cfg_key, options, props):
        assert (self.submit_url)

        # Properties prefix
        props_prefix = "auto_options_%d_" % (FormHelper.autoops_pre)
        FormHelper.autoops_pre += 1

        # The Table entry itself
        js = "options_changed('/ajax/update','%s', '%s');" % (cfg_key, props_prefix)
        self.AddTableOptions (table, title, cfg_key, options, 
                              onChange=js, wrap_id=props_prefix)

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
    
    def Validate_NotEmpty (self, post, cfg_key, error_msg):
        try:
            cfg_val = self._cfg[cfg_key].value
        except:
            cfg_val = None

        if not cfg_val and \
           not post.get_val(cfg_key):
            self._error_add (cfg_key, '', error_msg)

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
        checkboxes_pre = ["%s!%s"%(prefix, x) for x in checkboxes]
        return self.ApplyChanges (checkboxes_pre, post, validation)

    def ApplyChanges_OptionModule (self, cfg_key, uri, post):
        try:
            name = self._cfg[cfg_key].value
        except:
            return

        # Instance module and apply the changes
        module = module_obj_factory (name, self._cfg, cfg_key, self.submit_url)
        module._op_apply_changes (uri, post)

        # Include module errors
        for error in module.errors:
            self.errors[error] = module.errors[error]

        # Clean up properties
        props = module.__class__.PROPERTIES

        to_be_deleted = []
        for entry in self._cfg[cfg_key]:
            prop_cfg = "%s!%s" % (cfg_key, entry)
            if entry not in props:
                to_be_deleted.append (prop_cfg)

        for entry in to_be_deleted:
            del(self._cfg[entry])

