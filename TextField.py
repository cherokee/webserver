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
from Table import Table
from RawHTML import RawHTML


class TextField (Widget):
    def __init__ (self, props={}):
        Widget.__init__ (self)
        self._props = props

        if not 'id' in props:
            self._props['id'] = 'widget%d'%(self.uniq_id)

    def __get_common_props (self, skip_props=[]):
        render = ''

        for prop in self._props:
            if prop in skip_props:
                continue

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
        render = '<input type="text"%s>' % (self.__get_common_props())

        # Render the error reporting field
        render += '<div class="error"%s>' % (self.__get_common_props(['class']))

        return render

