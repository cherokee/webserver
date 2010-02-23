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

from Widget import Widget


class HiddenField (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)

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
                if type(value) == str:
                    render += '="%s"' %(value)
                else:
                    render += '=' + value
        return render

    def Render (self):
        # Render the text field
        html = '<input type="hidden"%s />' %(self.__get_input_props())

        render = Widget.Render(self)
        render.html += html

        return render
