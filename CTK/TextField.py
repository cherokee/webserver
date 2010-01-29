# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009 Alvaro Lopez Ortega
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

from Table import Table
from RawHTML import RawHTML
from Widget import Widget, RenderResponse
from Server import cfg

HEADER = [
    '<script type="text/javascript" src="/CTK/js/jquery.form-defaults.js"></script>'
]


class TextField (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)
        self.type = "text"

        if props:
            self._props = props
        else:
            self._props = {}

        if not 'id' in self._props:
            self._props['id'] = 'widget%d'%(self.uniq_id)

    def __get_input_props (self):
        render = ''
        for prop in self._props:
            render += " %s" %(prop)
            value = self._props[prop]
            if value:
                render += '="%s"' %(str(value))
        return render

    def __get_error_div_props (self):
        render = ' id="error_%s"' % (self._props['id'])
        if self._props.get('name'):
            render += ' key="%s"' %(self._props.get('name'))
        return render

    def Render (self):
        # Render the text field
        html = '<input type="%s"%s />' %(self.type, self.__get_input_props())

        # Render the error reporting field
        html += '<div class="error"%s></div>' %(self.__get_error_div_props())

        # Watermark
        js = ''
        if self._props.get('optional'):
            js += "$('#%s').DefaultValue('optional','%s');" %(
                self._props.get('id'), _("optional"))

        return RenderResponse (html, js, headers=HEADER)


class TextFieldPassword (TextField):
    def __init__ (self, *a, **kw):
        TextField.__init__ (self, *a, **kw)
        self.type = "password"

class TextCfg (TextField):
    def __init__ (self, key, optional=False, props=None):
        if not props:
            props = {}

        # Read the key value
        val = cfg.get_val(key)
        if val:
            props['value'] = val

        if optional:
            props['optional'] = True

        # Other properties
        props['name'] = key

        # Init parent
        TextField.__init__ (self, props)
