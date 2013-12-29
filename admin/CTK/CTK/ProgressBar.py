# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010-2014 Alvaro Lopez Ortega
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

import os
from Widget import Widget
from util import props_to_str

HEADERS = [
    '<link type="text/css" href="/CTK/css/CTK.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/CTK/js/jquery-ui-1.7.2.custom.min.js"></script>'
]

HTML = """
<div id="%(id)s" %(props)s></div>
"""

PERCENT_INIT_JS = """
$('#%(id)s').progressbar({ value: %(value)s });
"""

class ProgressBar (Widget):
    def __init__ (self, props={}):
        Widget.__init__ (self)
        self.id    = "progressbar_%d" %(self.uniq_id)
        self.value = props.pop ('value', 0)

        self.props = props.copy()
        if 'class' in props:
            self.props['class'] += ' progressbar'
        else:
            self.props['class'] = 'progressbar'

    def Render (self):
        render = Widget.Render (self)

        props = {'id':    self.id,
                 'value': self.value,
                 'props': props_to_str (self.props)}

        render.html    += HTML %(props)
        render.js      += PERCENT_INIT_JS %(props)
        render.headers += HEADERS

        return render

    def JS_to_set (self, value):
        return "$('#%s').progressbar ('option', 'value', %s);" %(self.id, value)
