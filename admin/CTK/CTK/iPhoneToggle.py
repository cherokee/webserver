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
from util import *

HEADERS = [
    '<script type="text/javascript" src="/CTK/js/jquery.ibutton.js"></script>',
    '<link type="text/css" href="/CTK/css/jquery.ibutton.css" rel="stylesheet" />'
]

HTML = """
<input type="checkbox" id="%(id)s" %(props)s/>
"""

JS = """
$("#%(id)s").iButton();
"""

class iPhoneToggle (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)

        if props:
            self.props = props
        else:
            self.props = {}

        if 'id' in self.props:
            self.id = self.props.pop('id')

    def Render (self):
        render = Widget.Render(self)

        props = {'id':    self.id,
                 'props': props_to_str(self.props)}

        render.html    += HTML %(props)
        render.js      += JS   %(props)
        render.headers += HEADERS

        return render

class iPhoneCfg (iPhoneToggle):
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
        iPhoneToggle.__init__ (self, props)
