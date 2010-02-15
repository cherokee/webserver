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

__author__ = 'Alvaro Lopez Ortega <alvaro@alobbs.com>'

from Widget import Widget, RenderResponse
from Server import cfg

HTML = """
<input type="checkbox" name="%(name)s" %(checked)s %(disabled)s/>
"""

class Checkbox (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)

        if props:
            self._props = props
        else:
            self._props = {}

        checked = props.pop('checked')

        self.checked  = bool(int(checked))
        self.disabled = props.get('disabled')

    def Render (self):
        if self.checked:
            self._props['checked'] = 'checked="checked" '
        else:
            self._props['checked'] = ''

        if self.disabled:
            self._props['disabled'] = 'disabled'
        else:
            self._props['disabled'] = ''

        # Render the text field
        html = HTML % (self._props)
        return RenderResponse (html)

class CheckCfg (Checkbox):
    def __init__ (self, key, default, props=None):
        if not props:
            props = {}

        # Read the key value
        val = cfg.get_val(key)
        if not val:
            props['checked'] = "01"[bool(int(default))]
        elif val.isdigit():
            props['checked'] = "01"[bool(int(val))]
        else:
            assert False, "Could not handle value: %s"%(val)

        # Other properties
        props['name'] = key

        # Init parent
        Checkbox.__init__ (self, props)
