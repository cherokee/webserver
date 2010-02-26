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
from Container import Container


class Box (Container):
    def __init__ (self, props=None, content=None):
        Container.__init__ (self)

        # Object ID
        if props and 'id' in props:
            self.id = props.pop('id')

        # Initial value
        if content:
            if isinstance (content, Widget):
                self += content
            elif type(content) in (list, type):
                for o in content:
                    self += o
            else:
                raise TypeError, 'Unknown type: "%s"' %(type(content))

    def Render (self):
        render = Container.Render (self)
        render.html = '<div id="%s">%s</div>' %(self.id, render.html)
        return render
