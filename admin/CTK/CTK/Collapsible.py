# -*- coding: utf-8 -*-
#
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

from Box import Box
from Widget import Widget
from RawHTML import RawHTML

class Collapsible (Box):
    """
    Widget to show content in an area that can be collapsed and
    expanded on-click.

       Arguments:
           titles: Tuple containing two strings to use as
               titles. Those are shown when the collpasible is
               minimized and maximized respectively.

           collapsed: Boolean to indicate initial state. Defaults to
               True.

       Examples:
          container  = CTK.Collapsible (('Show', 'Hide'))
          container += CTK.RawHTML ('<p>This text can be hidden, or not.</p>')
    """
    def __init__ (self, (titles), collapsed=True):
        Box.__init__ (self, {'class': 'collapsible'})

        self.collapsed = collapsed
        if collapsed:
            self.content = Box ({'class': 'collapsible-content', 'style': 'display:none;'})
        else:
            self.content = Box ({'class': 'collapsible-content'})

        assert len(titles) == 2
        self.title_show = Box ({'class': 'collapsible-title'}, titles[0])
        self.title_hide = Box ({'class': 'collapsible-title'}, titles[1])

        self.title_show.bind ('click', self.__JS_show())
        self.title_hide.bind ('click', self.__JS_hide())

        # Build up
        Box.__iadd__ (self, self.title_show)
        Box.__iadd__ (self, self.title_hide)
        Box.__iadd__ (self, self.content)

    def __iadd__ (self, content):
        self.content += content
        return self

    def __JS_show (self):
        return self.content.JS_to_show(100) + self.title_hide.JS_to_show() + self.title_show.JS_to_hide()

    def __JS_hide (self):
        return self.content.JS_to_hide() + self.title_hide.JS_to_hide() + self.title_show.JS_to_show()

    def Render (self):
        render = Box.Render (self)

        if self.collapsed:
            render.js += self.__JS_hide()
        else:
            render.js += self.__JS_show()

        return render

class CollapsibleEasy (Collapsible):
    """
    Widget to show content in an area that can be collapsed and
    expanded on-click. It works just like the Collapsible, but
    prepends up/down arrows to the titles to indicate collapse/expand.

       Arguments:
           titles: Tuple containing two strings to use as
               titles. Those are shown when the collpasible is
               minimized and maximized respectively.

           collapsed: Boolean to indicate initial state. Defaults to
               True.

       Examples:
          container  = CTK.CollapsibleEasy (('Show', 'Hide'))
          container += CTK.RawHTML ('<p>This text can be hidden, or not.</p>')
    """
    def __init__ (self, (titles), collapsed=True):
        assert len(titles) == 2
        assert type(titles[0]) == str
        assert type(titles[1]) == str

        if collapsed:
            c0 = "▼ "
            c1 = "▲ "
        else:
            c0 = "▲ "
            c1 = "▼ "

        show = RawHTML(c0 + titles[0])
        hide = RawHTML(c1 + titles[1])
        Collapsible.__init__ (self, (show, hide), collapsed)
