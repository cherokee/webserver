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

from html import escape

from .Widget import Widget
from .Server import cfg
from .i18n import _
from .util import props_to_str
from .util import to_utf8

HEADER = [
    '<script type="text/javascript" src="/CTK/js/jquery.form-defaults.js"></script>'
]

class TextArea (Widget):
    """
    Widget for <textarea> element. Arguments are optional.

       Arguments:
           props: dictionary with properties for the HTML element,
               such as {'name': 'foo', 'id': 'bar', 'class': 'baz'}.
           content: text initially contained in the textarea

       Examples:
          area = CTK.TextArea({'name': 'report'}, 'Report your problem')
    """

    def __init__ (self, props=None, content=''):
        Widget.__init__ (self)
        self._props   = (props or {}).copy()
        self._content = content

        if 'id' in self._props:
            self.id = self._props.pop('id')

    def __get_input_props (self):
        render = ''
        for prop in self._props:
            render += f" {prop}"
            value = self._props[prop]
            if value:
                val = to_utf8(value)
                render += f'="{val}"'
        return render

    def __get_error_div_props (self):
        render = f'id="error_{self.id}"'
        name = to_utf8 (self._props.get('name'))
        if name:
            render += f' key="{name}"'
        return render


    def Render (self):
        # Watermark
        js = ''

        if self._props.get('optional'):
            optional_string = self._props.get('optional_string', _("Optional"))
            js += f"$('#{self.id}').DefaultValue('optional-grey','{optional_string}');"

            if not self._props.get('class'):
                self._props['class'] = 'optional'
            else:
                self._props['class'] += ' optional'

        # Render the text field
        html = f'<textarea id="{self.id}"{self.__get_input_props()}>{self._content}</textarea>'

        # Render the error reporting field
        html += f'<div class="error" {self.__get_error_div_props()}></div>'

        render = Widget.Render (self)
        render.html    += html
        render.js      += js
        render.headers += HEADER

        return render

    def JS_to_clean (self):
        return f"$('#{self.id}').val('');"
    def JS_to_focus (self):
        return f"$('#{self.id}').blur(); $('#{self.id}').focus();"

class TextAreaCfg (TextArea):
    """
    Configuration-Tree based TextArea widget. Populates the input
    field with the value of the configuration tree given by key
    argument if it exists. It can be set as optional, and accepts
    properties to pass to the base TextArea object.
    """
    def __init__ (self, key, optional=False, props={}, content=''):
        # Sanity checks
        assert type(key) == str
        assert type(optional) == bool
        assert type(props) in (type(None), dict)

        props = props.copy()

        # Read the key value
        val = cfg.get_val(key)
        if val:
            content = escape(val, quote=False)

        if optional:
            props['optional'] = True

        # Other properties
        props['name'] = key

        # Init parent
        TextArea.__init__ (self, props, content)
