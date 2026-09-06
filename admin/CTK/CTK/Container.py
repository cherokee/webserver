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

from __future__ import annotations

from typing import TypeVar

from .Widget import RenderResponse, Widget

ContainerT = TypeVar('ContainerT', bound='Container')

class Container (Widget):
    """
    Base container widget. Calling the Render method will produce the
    HTML associated with the widgets contained within. Thus, if no
    such widget exists, no HTML will be rendered.
    """
    def __init__ (self):
        Widget.__init__ (self)
        self.child: list[Widget] = []

    def __getitem__ (self, n: int) -> Widget:
        return self.child[n]

    def __len__ (self):
        return len(self.child)

    def __bool__ (self):
        # It's an obj, no matter its child.
        return True

    def __iadd__ (self: ContainerT, widget: Widget) -> ContainerT:
        if not isinstance(widget, Widget):
            raise TypeError('Container children must be Widget instances')
        self.child.append (widget)
        return self

    def Empty (self):
        self.child = []

    def Render (self) -> RenderResponse:
        render = Widget.Render(self)

        for c in self.child:
            tmp = c.Render()
            render += tmp

        return render
