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
from Server import cfg


class Combobox (Widget):
    def __init__ (self, props, options):
        Widget.__init__ (self)

        self.props    = props.copy()
        self._options = options

        if not 'id' in self.props:
            self.props['id'] = 'Combobox_%s' %(self.uniq_id)
        self.id = self.props['id']

    def Render (self):
        selected = self.props.get('selected')

        # Render entries
        content = ''
        for o in self._options:
            name, label = o
            if selected and str(selected) == str(name):
                content += '<option value="%s" selected="true">%s</option>' % (name, label)
            else:
                content += '<option value="%s">%s</option>' % (name, label)

        # Render the container
        header = ''
        for p in filter(lambda x: x!='selected', self.props):
            if self.props[p]:
                header += ' %s="%s"' %(p, self.props[p])
            else:
                header += ' %s' %(p)

        html = '<select%s>%s</select>' %(header, content)

        render = Widget.Render (self)
        render.html += html

        return render


class ComboCfg (Combobox):
    def __init__ (self, key, options, _props={}):
        props = _props.copy()

        # Read the key value
        val = cfg.get_val(key)
        sel = None

        # Look for the selected entry
        for v,k in options:
            if v == val:
                sel = val

        if sel:
            props['selected'] = sel

        # Other properties
        props['name'] = key

        # Init parent
        Combobox.__init__ (self, props, options)
