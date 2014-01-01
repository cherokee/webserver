# -*- coding: utf-8 -*-

# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009-2014 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

from cgi import escape

from RawHTML import RawHTML
from Widget import Widget
from Server import cfg
from util import to_utf8
from Server import get_server

HEADER = [
    '<script type="text/javascript" src="/CTK/js/jquery.form-defaults.js"></script>'
]

class TextField (Widget):
    """
    Widget for the base <input type="text"> element. Arguments are optional.

       Arguments:
           props: dictionary with properties for the HTML element,
               such as {'name': 'foo', 'id': 'bar', 'class': 'noauto
               required', optional: 'True', optional_string: 'Whatever
               works...'}.

               noauto: form will not be submitted instantly upon
                   edition of the field.

               optional: True|False, defines whether a grayed out
                   'Optional' string should be overlayed on the input field.

               optional_string: if optional flag is enabled, show
                   this string instead of 'Optional'.

       Examples:
          field1 = CTK.TextField({'name': 'email1', 'class': 'noauto'})

          field2 = CTK.TextField({'name': 'email2', 'class': 'noauto',
                                  'optional': True, 'optional_string':
                                  'Providing this field is optional}})
    """
    def __init__ (self, props=None):
        Widget.__init__ (self)
        self.type = "text"
        self.id   = 'widget%d'%(self.uniq_id)

        if props:
            self._props = props
        else:
            self._props = {}

        if not 'id' in self._props:
            self._props['id'] = self.id

    def __get_input_props (self):
        render = ''
        for prop in self._props:
            render += " %s" %(prop)
            value = self._props[prop]
            if value:
                val = to_utf8(value)
                render += '="%s"' %(val)
        return render

    def __get_error_div_props (self):
        render = 'id="error_%s"' % (self.id)
        name = to_utf8 (self._props.get('name'))
        if name:
            render += ' key="%s"' %(name)
        return render

    def Render (self):
        # Watermark
        js = ''

        if self._props.get('optional'):
            optional_string = self._props.get('optional_string', _("Optional"))
            js += "$('#%s').DefaultValue('optional-grey','%s');" %(self.id, optional_string)

            if not self._props.get('class'):
                self._props['class'] = 'optional'
            else:
                self._props['class'] += ' optional'

        # Render the text field
        html = '<input type="%s"%s />' %(self.type, self.__get_input_props())

        # Render the error reporting field
        html += '<div class="error" %s></div>' %(self.__get_error_div_props())

        render = Widget.Render (self)
        render.html    += html
        render.js      += js
        render.headers += HEADER

        return render

    def JS_to_clean (self):
        return "$('#%s').attr('value', '');" %(self.id)
    def JS_to_focus (self):
        return "$('#%s').blur(); $('#%s').focus();" %(self.id, self.id)


class TextFieldPassword (TextField):
    """
    Widget for password fields. Behaves exactly like a TextField, but
    renders a <input type="password> element instead.
    """
    def __init__ (self, *a, **kw):
        TextField.__init__ (self, *a, **kw)
        self.type = "password"


class TextCfg (TextField):
    """
    Configuration-Tree based TextField widget. Populates the input
    field with the value of the configuration tree given by key
    argument if it exists. It can be set as optional, and accepts
    properties to pass to the base TextField object.
    """
    def __init__ (self, key, optional=False, props={}):
        # Sanity checks
        assert type(key) == str
        assert type(optional) == bool
        assert type(props) in (type(None), dict)

        props = props.copy()

        # Read the key value
        val = cfg.get_val(key)
        if val:
            props['value'] = escape(val, quote=True)

        if optional:
            props['optional'] = True

        # Other properties
        props['name'] = key

        # Init parent
        TextField.__init__ (self, props)


class TextCfgPassword (TextCfg):
    """
    Configuration-Tree based TextPassword widget. Populates the input
    field with the value of the configuration tree given by key
    argument if it exists. It can be set as optional, and accepts
    properties to pass to the base TextPassword object.
    """
    def __init__ (self, *a, **kw):
        TextCfg.__init__ (self, *a, **kw)
        self.type = "password"



JS = """
$("#%(id)s")

.bind ("blur", function (event){
    var self  = $(this);
    var value = self.val();

    if (value != self.data('last_value')) {
       $("#activity").show();

       $.ajax ({type:     'POST',
                dataType: 'json',
                url:      '%(url)s',
                data:     '%(key)s='+value,
                success: function (data) {
                    self.data('last_value', value);

                    var name = self.attr('name');
                    for (var key in data['updates']) {
                       if (key == name) {
                          self.val (data['updates'][key]);
                          break;
                       }
                    }
                },
                error: function() {
                    event.stopPropagation();
                    self.val (self.data('last_value'));
                },
                complete: function (XMLHttpRequest, textStatus) {
                    $("#activity").fadeOut('fast');
                }
       });
    }
})

.data ('last_value', "%(value)s")

.bind ("keypress", function(event){
    if (event.keyCode == 13) {
        focus_next_input (this);
    }
});
"""

class TextCfgAuto (TextCfg):
    """
    Automatic variant of TextCfg widget. It needs a key for an entry
    in the Configuration tree, and an url to which data is sent by
    HTTP POST. It can be set as optional, and accepts properties to
    pass to the base TextField object.

    The field is automatically populated by the value of the
    configuration entry given by key, and the configuration entry is
    updated automatically when the input field is submitted.
    """
    def __init__ (self, key, url, optional=False, props=None):
        self.key = key
        self.url = url
        TextCfg.__init__ (self, key, optional, props)

        # Secure submit
        srv = get_server()
        if srv.use_sec_submit:
            self.url += '?key=%s' %(srv.sec_submit)

    def Render (self):
        value = cfg.get_val (self.key, '')

        render = TextCfg.Render(self)
        render.js += JS %({'id':    self.id,
                           'url':   self.url,
                           'key':   self.key,
                           'value': value})
        return render
