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

class Image (Widget):
    def __init__ (self, props=None, **kwargs):
        Widget.__init__ (self, **kwargs)
        if props:
            self.props = props
        else:
            self.props = {}

    def Render (self):
        if not 'id' in self.props:
            self.props['id'] = self.id

        props = " ".join (['%s="%s"'%(k,self.props[k]) for k in self.props])

        render = Widget.Render (self)
        render.html = '<img %s />' %(props)
        return render
