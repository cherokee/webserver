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

from Widget import Widget
from Server import cfg

HTML = """
<input type="checkbox" %(props)s />
"""

class Checkbox (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)

        if props:
            self._props = props
        else:
            self._props = {}

    def Render (self):
        # Deal with a couple of exceptions
        if self._props.has_key('checked') and int(self._props.pop('checked')):
            self._props['checked'] = "checked"

        if self._props.has_key('disabled'):
            self._props['disabled'] = None

        # Render the properties
        plist = []
        for k in self._props:
            if self._props[k]:
                plist.append ('%s="%s"'%(k, self._props[k]))
            else:
                plist.append (k)
        props = " ".join (plist)

        # Render the widget
        render = Widget.Render (self)
        render.html += HTML % ({'props': props})
        return render


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
