# -*- coding: utf-8 -*-
#
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
    def __init__ (self, props={}, **kwargs):
        Widget.__init__ (self, **kwargs)
        self.props = props.copy()

    def Render (self):
        if not 'id' in self.props:
            self.props['id'] = self.id

        props = " ".join (['%s="%s"'%(k,self.props[k]) for k in self.props])

        render = Widget.Render (self)
        render.html = '<img %s />' %(props)
        return render

class ImageStock (Image):
    def __init__ (self, name, _props={}):
        props = _props.copy()

        if name == 'del':
            props['src']   = '/CTK/images/del.png'
            props['alt']   = _('Delete')
            props['title'] = _('Delete')

            if 'class' in props:
                props['class'] += ' del'
            else:
                props['class'] = 'del'

            Image.__init__ (self, props)

        elif name == 'on':
            props['src'] = '/CTK/images/on.png'
            props['alt'] = _('Active')
            Image.__init__ (self, props)

        elif name == 'off':
            props['src'] = '/CTK/images/off.png'
            props['alt'] = _('Inactive')
            Image.__init__ (self, props)

        elif name == 'loading':
            props['src'] = '/CTK/images/loader.gif'
            props['alt'] = _('Loading')
            Image.__init__ (self, props)

        elif name == 'tick':
            props['src'] = '/CTK/images/tick.png'
            props['alt'] = _('Enabled')
            Image.__init__ (self, props)

        else:
            assert False, "Unknown stock image: %s" %(name)
