import re
import types

from Entry import *
from Module import *

FORM_TEMPLATE = """
<form action="%(action)s" method="%(method)s">
  %(content)s
  %(submit)s
  <input type="hidden" name="is_submit" value="1" />
</form>
"""

AUTO_SUBMIT_JS = """
<script type="text/javascript">
$(document).ready(function(event) {
  $(".auto input").change(function(event) {
  if (check_all_or_none('required'))
      this.form.submit()
  });
});
</script>
"""

AUTOFORM_TEMPLATE = FORM_TEMPLATE.replace('<form','<form class="auto"') + AUTO_SUBMIT_JS

SUBMIT_BUTTON = """
<input type="submit" %(submit_props)s />
"""

DEFAULT_SUBMIT_VALUE= 'value="Submit"'

class WebComponent:
    def __init__ (self, id, cfg):
        self._id  = id
        self._cfg = cfg

    def _op_handler (self, uri, post):
        if post.get_val('is_submit'):
            tmp = self._op_apply_changes (uri, post)
            if tmp:
                return tmp
        return self._op_render()

    def _op_render (self):
        raise "Should have been overridden"

    def _apply_changes (self, uri, post):
        raise "Should have been overridden"

    def HandleRequest (self, uri, post):
        # Is this a form submit
        is_submit = post.get_val('is_submit')

        # Check the URL
        parts = uri.split('/')[2:]
        if parts:
            ruri = '/' + '/'.join(parts)
        else:
            ruri = '/'

        if len(ruri) <= 1 and not is_submit:
            return self._op_render()

        return self._op_handler(ruri, post)

class Form:
    def __init__ (self, action, method='post', add_submit=True, auto=True):
        self._action       = action
        self._method       = method
        self._add_submit   = add_submit
        self._auto         = auto

    def Render (self, content='', submit_props='' ):
        keys = {'submit':       '',
                'submit_props': submit_props,
                'content':      content,
                'action':       self._action,
                'method':       self._method}

        if self._add_submit:
            keys['submit'] = SUBMIT_BUTTON

        if self._auto:
            render = AUTOFORM_TEMPLATE
        else:
            render = FORM_TEMPLATE

        while '%(' in render:
            for replacement in re.findall (r'\%\((\w+)\)s', render):
                macro = '%('+replacement+')s'
                render = render.replace (macro, keys[replacement])

        return render


class FormHelper (WebComponent):
    options_wrap_num = 1

    def __init__ (self, id, cfg):
        WebComponent.__init__ (self, id, cfg)

        self.errors      = {}
        self.submit_url  = None

    def set_submit_url (self, url):
        self.submit_url = url

    def Indent (self, content):
        return '<div class="indented">%s</div>' %(content)

    def Dialog (self, txt, type_='information'):
        return '<div class="dialog-%s">%s</div>' % (type_, txt)

    def HiddenInput (self, key, value):
        return '<input type="hidden" value="%s" id="%s" name="%s" />' % (value, key, key)

    def InstanceEntry (self, cfg_key, type, **kwargs):
        # Instance an Entry
        entry = Entry (cfg_key, type, self._cfg, **kwargs)
        txt = str(entry)

        # Check whether there is a related error
        if cfg_key in self.errors.keys():
            msg, val = self.errors[cfg_key]
            if val:
                txt += '<div class="error"><b>%s</b>: %s</div>' % (val, msg)
            else:
                txt += '<div class="error">%s</div>' % (msg)
        return txt

    def Label(self, title, for_id):
        txt = '<label for="%s">%s</label>' % (for_id, title)
        return txt

    def InstanceTab (self, entries):
        # HTML
        txt = '<dl class="tab" id="tab_%s">' % (self._id)
        num = 0
        for title, content in entries:
            txt += '<dt num="%d">%s</dt>\n' % (num, title)
            txt += '<dd>%s</dd>\n' % (content)
            num += 1
        txt += '</dl>'

        # Javascript
        txt += '''
        <script type="text/javascript">
          var settings = {
             autoheight: true,
             alwaysOpen: true,
             animated:   false
          };

          open_tab = get_cookie('open_tab');
          if (open_tab) {
            settings['active'] = parseInt(open_tab);
          }

          jQuery("#tab_%s").Accordion(settings).change(
            function (event, newHeader, oldHeader) {
              if (! newHeader) return;
              document.cookie = "open_tab=" + newHeader.attr("num");
          });
        </script>
        ''' % (self._id)
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

    def InstanceImage (self, name, alt, **kwargs):
        extra = ""
        for karg in kwargs:
            extra += '%s="%s" '%(karg, kwargs[karg])
        return '<img src="/static/images/%s" alt="%s" %s/>' % (name, alt, extra)

    def _get_auto_wrap_id (self):
        return "options_wrap_%d" % (FormHelper.options_wrap_num)

    def InstanceOptions (self, cfg_key, options, *args, **kwargs):
        value = self._cfg.get_val (cfg_key)
        if value != None:
            ops = EntryOptions (cfg_key, options, selected=value, *args, **kwargs)
        else:
            ops = EntryOptions (cfg_key, options, *args, **kwargs)

        # Auto wrap
        auto_wrap_id = self._get_auto_wrap_id()
        FormHelper.options_wrap_num += 1

        ops = '<div id="%s" name="%s">%s</div>'%(auto_wrap_id, auto_wrap_id, ops)
        return (ops, value, auto_wrap_id)

    def AddTableOptions (self, table, title, cfg_key, options, *args, **kwargs):
        entry, value, wrap = self.InstanceOptions (cfg_key, options, *args, **kwargs)

        label = self.Label(title, cfg_key)
        table += (label, entry)

        return value

    def AddTableOptions_Ajax (self, table, title, cfg_key, options, *args, **kwargs):
        wrap_id = self._get_auto_wrap_id()
        js = "options_changed('/ajax/update','%s','%s');" % (cfg_key, wrap_id)
        kwargs['onChange'] = js

        return self.AddTableOptions (table, title, cfg_key, options, *args, **kwargs)

    def AddPropOptions_Ajax (self, table, title, cfg_key, options, comment, *args, **kwargs):
        wrap_id = self._get_auto_wrap_id()
        js = "options_changed('/ajax/update','%s','%s');" % (cfg_key, wrap_id)
        kwargs['onChange'] = js

        return self.AddPropOptions (table, title, cfg_key, options, comment, *args, **kwargs)

    def AddPropOptions_Reload (self, table, title, cfg_key, options, comment, **kwargs):
        assert (self.submit_url)

        # The Table entry itself
        auto_wrap_id = self._get_auto_wrap_id()
        js = "options_changed('/ajax/update','%s','%s');" % (cfg_key, auto_wrap_id)
        kwargs['onChange'] = js
        name = self.AddPropOptions (table, title, cfg_key, options, comment, **kwargs)

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

    def AddTableOptions_Reload (self, table, title, cfg_key, options, **kwargs):
        assert (self.submit_url)
        print "DEPRECATED: AddTableOptions_Reload"

        # The Table entry itself
        auto_wrap_id = self._get_auto_wrap_id()
        js = "options_changed('/ajax/update','%s','%s');" % (cfg_key, auto_wrap_id)
        name = self.AddTableOptions (table, title, cfg_key, options, onChange=js)

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

    def InstanceCheckbox (self, cfg_key, default=None, quiet=False):
        try:
            tmp = self._cfg[cfg_key].value.lower()
            if tmp in ["on", "1", "true"]:
                value = '1'
            else:
                value = '0'
        except:
            value = None

        if value == '1':
            entry = Entry (cfg_key, 'checkbox', quiet=quiet, checked=value)
        elif value == '0':
            entry = Entry (cfg_key, 'checkbox', quiet=quiet)
        else:
            if default == True:
                entry = Entry (cfg_key, 'checkbox', quiet=quiet, checked='1')
            elif default == False:
                entry = Entry (cfg_key, 'checkbox', quiet=quiet)
            else:
                entry = Entry (cfg_key, 'checkbox', quiet=quiet)

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

    def ValidateChange_SingleKey (self, key, post, validation):
        for regex, tmp in validation:
            pass_cfg = False

            if type(tmp) == types.FunctionType:
                validation_func = tmp

            elif type(tmp) == types.TupleType:
                validation_func = tmp[0]
                for k in tmp[1:]:
                    if k == 'cfg':
                        pass_cfg = True
                    else:
                        print "UNKNOWN validation option:", k

            p = re.compile (regex)
            if p.match (key):
                value = post.get_val(key)
                if not value:
                    continue
                try:
                    if pass_cfg:
                        tmp = validation_func (value, self._cfg)
                    else:
                        tmp = validation_func (value)
                    post[key] = [tmp]
                except ValueError, error:
                    self._error_add (key, value, error)

    def _ValidateChanges (self, post, validation):
        for post_entry in post:
            self.ValidateChange_SingleKey (post_entry, post, validation)

    def ApplyCheckbox (self, post, cfg_key):
        if cfg_key in self.errors:
            return

        if cfg_key in post:
            value = post.pop(cfg_key)
            if value.lower() in ['on', '1']:
                self._cfg[cfg_key] = '1'
            else:
                self._cfg[cfg_key] = '0'
        else:
            self._cfg[cfg_key] = "0"

    def ApplyChanges (self, checkboxes, post, validation=None):
        # Validate changes
        if validation:
            self._ValidateChanges (post, validation)

        # Apply checkboxes
        for key in checkboxes:
            self.ApplyCheckbox (post, key)

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

    def ApplyChangesDirectly (self, post):
        for confkey in post:
            if not '!' in confkey:
                continue

            if confkey in self.errors:
                continue

            value = post[confkey][0]
            if not value:
                del (self._cfg[confkey])
            else:
                self._cfg[confkey] = value

    def ApplyChangesPrefix (self, prefix, checkboxes, post, validation=None):
        checkboxes_pre = ["%s!%s"%(prefix, x) for x in checkboxes]
        return self.ApplyChanges (checkboxes_pre, post, validation)

    def ApplyChanges_OptionModule (self, cfg_key, uri, post):
        # Read the option entry value
        name = self._cfg.get_val(cfg_key)
        if not name: return

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

    def AddProp (self, table, title, cfg_key, entry, comment=None):
        label = self.Label (title, cfg_key);
        table += (label, entry, comment)

    def AddPropEntry (self, table, title, cfg_key, comment=None, **kwargs):
        entry = self.InstanceEntry (cfg_key, 'text', **kwargs)
        self.AddProp (table, title, cfg_key, entry, comment)

    def AddPropCheck (self, table, title, cfg_key, default, comment=None):
        entry = self.InstanceCheckbox (cfg_key, default)
        self.AddProp (table, title, cfg_key, entry, comment)

    def AddPropOptions (self, table, title, cfg_key, options, comment=None, **kwargs):
        entry, v, w = self.InstanceOptions (cfg_key, options, **kwargs)
        self.AddProp (table, title, cfg_key, entry, comment)
        return v


class TableProps:
    def __init__ (self):
        self._content = []

    def __add__ (self, (title, content, comment)):
        self._content.append((title, content, comment))

    def _render_entry (self, (title, content, comment)):
        txt  = '<table class="tableprop">'
        txt += '  <tr><th class="title">%s</th><td>%s</td></tr>' % (title, content)
        txt += '  <tr><td colspan="2"><div class="comment">%s</div></td></tr>' % (comment)
        txt += '</table>'
        return txt

    def __str__ (self):
        tmp = []
        for entry in self._content:
            tmp.append (self._render_entry(entry))

        txt = "<hr />".join(tmp)
        return '<div class="tableprop_block">%s</div>' % (txt)
                        
