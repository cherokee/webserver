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
    '<script type="text/javascript" src="/CTK/js/jquery.ibutton.js"></script>',
    '<link type="text/css" href="/CTK/css/jquery.ibutton.css" rel="stylesheet" />'
]

HTML = """
<input type="checkbox" id="%(id)s" name="%(name)s" value="true" />
"""

JS = """
$("#%(id)s").iButton();
"""

class iPhoneToggle (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)

        if props:
            self._props = props
        else:
            self._props = {}

        if not 'id' in self._props:
            self._props['id'] = 'widget%d'%(self.uniq_id)

    def Render (self):
        render = Widget.Render(self)

        render.html += HTML %(self._props)
        render.js   += JS   %(self._props)

        return render
