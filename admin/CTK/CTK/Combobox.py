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
from Server import cfg
from util import props_to_str

class Combobox (Widget):
    """
    Widget for drop-down combo elements.

        Arguments:
            props: dictionary with properties for the HTML element,
                such as {'name': 'foo', 'id': 'bar', 'class': 'noauto'}.

                options: set of tuples in the form (value,
                    description), (v2, d2), ...

        Examples:
            combo = CTK.Combobox({'name': 'language'},
                                 [('en', 'English'), ('es', 'Spanish')])
    """
    def __init__ (self, props, options):
        Widget.__init__ (self)

        self.props    = props.copy()
        self._options = options

        if not 'id' in self.props:
            self.props['id'] = 'Combobox_%s' %(self.uniq_id)
        self.id = self.props['id']

    def Render (self):
        selected = self.props.get('selected')

        def render_str (o):
            if len(o) == 2:
                name, label = o
                props       = {}
            elif len(o) == 3:
                name, label, props = o

            props_str = props_to_str(props)
            if selected and str(selected) == str(name):
                return '<option value="%s" selected="true" %s>%s</option>' % (name, props_str, label)
            else:
                return '<option value="%s" %s>%s</option>' % (name, props_str, label)

        def render_list (o):
            if len(o) == 2:
                name, options = o
                props       = {}
            elif len(o) == 3:
                name, options, props = o

            props_str = props_to_str(props)
            txt = '<optgroup label="%s" %s>' %(name, props_str)
            for o in options:
                txt += render_str (o)
            txt += '</optgroup>'
            return txt

        # Render entries
        content = ''
        for o in self._options:
            if type(o[1]) == str:
                content += render_str (o)
            elif type(o[1]) == list:
                content += render_list (o)
            else:
                raise ValueError

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
    """
    Configuration-Tree based Combobox widget. Pre-selects the
    combo-entry corresponding to the value of the configuration tree
    given by key argument if it exists. Everything else is like the
    Combobox widget.
    """
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
