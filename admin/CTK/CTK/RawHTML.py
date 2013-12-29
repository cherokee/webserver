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

from Widget import Widget
from util import to_utf8


class RawHTML (Widget):
    """
    Widget for raw HTML contents. CTK Widgets can be added one to
    another, so in order to append HTML code directly to an existing
    CTK widget it has to be embedded into a RawHTML widget itself.
    Arguments are optional.

       Arguments:
          html: text string containing HTML code.
          js:   text string containing Javascript code.

       Examples:
          html1 = CTK.RawHTML('<h1>Header</h1>')
          html2 = CTK.RawHTML(js='window.location.reload();')
    """
    def __init__ (self, html='', js=''):
        Widget.__init__ (self)

        html = to_utf8(html)
        js   = to_utf8(js)

        # Since the information contained in this widget will most
        # probably go through a few variable replacement processes,
        # the % characters are converted to %% in order to avoid
        # potential issues during the process. The HTML code might
        # contain strings such as 'style: 100%;', which aren't meant
        # to be variable replacement.
        #
        html = html.replace ('%', '%%')
        js   =   js.replace ('%', '%%')

        self.html = html
        self.js   = js

    def __add__ (self, txt):
        assert type(txt) == str
        self.html += txt

    def Render (self):
        render = Widget.Render(self)
        render.html += self.html
        render.js   += self.js
        return render
