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

HEADERS = [
    '<link type="text/css" href="/CTK/css/CTK.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/CTK/js/jquery-ui-1.7.2.custom.min.js"></script>'
]

HTML = """
<input type="text" id="%(id)s" class="datepicker">
"""

JS = """
$("#%(id)s").datepicker();
"""

class DatePicker (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)

        if props and 'id' in props:
            self.id = props['id']

    def Render (self):
        render = Widget.Render (self)

        render.html    += HTML %({'id': self.id})
        render.js      += JS   %({'id': self.id})
        render.headers += HEADERS

        return render
