# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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
from Container import Container
from Server import cfg
from util import *


HTML = """
<input type="checkbox" id="%(id)s" %(props)s />
"""

class Checkbox (Widget):
    def __init__ (self, props={}):
        # Sanity check
        assert type(props) == dict

        Widget.__init__ (self)
        self._props = props.copy()

    def Render (self):
        # Deal with a couple of exceptions
        new_props = self._props.copy()

        if new_props.has_key('checked') and int(new_props.pop('checked')):
            new_props['checked'] = "checked"

        if new_props.has_key('disabled') and int(new_props.pop('disabled')):
            new_props['disabled'] = None

        # Render the widget
        render = Widget.Render (self)
        render.html += HTML % ({'id':    self.id,
                                'props': props_to_str (new_props)})
        return render


class CheckCfg (Checkbox):
    def __init__ (self, key, default, props=None):
        # Sanity checks
        assert type(key) == str
        assert type(default) == bool
        assert type(props) in (type(None), dict)

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


class CheckboxText (Checkbox):
    def __init__ (self, props=None, text='Enabled'):
        Checkbox.__init__ (self, props)
        self.text = text

    def Render (self):
        render = Checkbox.Render (self)
        render.html = '<div id="%s" class="checkbox-text">%s %s</div>' %(self.id, render.html, self.text)
        return render


class CheckCfgText (CheckCfg):
    def __init__ (self, key, default, text='Enabled', props=None):
        assert type(default) == bool
        assert type(text) == str
        assert type(props) in (dict, type(None))

        CheckCfg.__init__ (self, key, default, props)
        self.text = text

    def Render (self):
        render = CheckCfg.Render (self)
        render.html = '<div id="%s" class="checkbox-text">%s %s</div>' %(self.id, render.html, self.text)
        return render
