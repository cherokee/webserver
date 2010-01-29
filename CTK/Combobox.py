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

from Widget import Widget, RenderResponse
from Server import cfg


class Combobox (Widget):
    def __init__ (self, props, options):
        Widget.__init__ (self)

        self._props   = props
        self._options = options

    def Render (self):
        selected = self._props.get('selected')

        # Render entries
        content = ''
        for o in self._options:
            name, label = o
            if selected and selected == name:
                content += '<option value="%s" selected="true">%s</option>' % (name, label)
            else:
                content += '<option value="%s">%s</option>' % (name, label)

        # Render the container
        header = ''
        for p in filter(lambda x: x!='selected', self._props):
            if self._props[p]:
                header += ' %s="%s"' %(p, self._props[p])
            else:
                header += ' %s' %(p)

        html = '<select%s>%s</select>' %(header, content)
        return RenderResponse(html)

class ComboCfg (Combobox):
    def __init__ (self, key, options, props=None):
        if not props:
            props = {}

        # Read the key value
        val = cfg.get_val(key)
        sel = None

        # Look for the selected entry
        for v,k in options:
            if v == val:
                sel = val

        if sel:
            props['selected'] = sel

        # Init parent
        Combobox.__init__ (self, props, options)
