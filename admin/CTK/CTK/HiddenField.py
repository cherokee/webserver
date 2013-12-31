# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010-2014 Alvaro Lopez Ortega
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
from util import props_to_str

class HiddenField (Widget):
    def __init__ (self, props={}):
        Widget.__init__ (self)
        self.props = props.copy()

    def Render (self):
        # Render the text field
        html = '<input type="hidden" id="%s" %s/>' %(self.id, props_to_str(self.props))

        render = Widget.Render(self)
        render.html += html

        return render

class Hidden (HiddenField):
    """
    Widget for hidden input field. Requires a name for the
    field. Optional value and properties can be passed.
    """
    def __init__ (self, name, value='', _props={}):
        props = _props.copy()
        props['name']  = name
        props['value'] = value[:]

        HiddenField.__init__ (self, props)
