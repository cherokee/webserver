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

from Container import Container
from Widget import RenderResponse

HEADERS = [
    '<link type="text/css" href="/CTK/css/jquery-ui-1.7.2.custom.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/CTK/js/jquery-ui-1.7.2.custom.min.js"></script>'
]

HTML = """
<div id="%(id)s" title="%(title)s">
%(content)s
</div>
"""

JS = """
$("#%(id)s").dialog(%(dialog_props)s);
"""

def py2js_dic (d):
    js_pairs = []

    for key in d:
        val = d[key]
        if type(val) == bool:
            js_pairs.append ('%s: %s' %(key, ('false', 'true')[val]))
        elif type(val) == int:
            js_pairs.append ('%s: %d' %(key, val))
        elif type(val) == str:
            if '/* code */' in val:
                js_pairs.append ("%s: %s" %(key, val))
            else:
                js_pairs.append ("%s: '%s'" %(key, val))
        else:
            assert false, "Unknown data type"

    return "{%s}" % (", ".join(js_pairs))


class Dialog (Container):
    def __init__ (self, props=None):
        Container.__init__ (self)

        if props:
            self.props = props
        else:
            self.props = {}

        self.id      = 'dialog%d'%(self.uniq_id)
        self.title   = self.props.pop('title', '')
        self.buttons = []

        # Defaults
        if not 'modal' in self.props:
            self.props['modal'] = True
        if not 'resizable' in self.props:
            self.props['resizable'] = False

    def AddButton (self, caption, action):
        self.buttons.append ((caption, action))

    def Render (self):
        # Build buttons
        if self.buttons:
            self.props['buttons'] = self._render_buttons_property()

        # Render
        render_child = Container.Render (self)
        dialog_props = py2js_dic (self.props)

        props = {'id':           self.id,
                 'title':        self.title,
                 'content':      render_child.html,
                 'dialog_props': dialog_props}

        html = HTML %(props)
        js   = JS   %(props)

        render = RenderResponse (html)
        render.js      = render_child.js + js
        render.headers = render_child.headers + HEADERS

        return render

    def _render_buttons_property (self):
        buttons_js = []

        # Render the button entries
        for tmp in self.buttons:
            button_caption, action = tmp
            tmp = '"%(button_caption)s": function() {'
            if action == "close":
                tmp += '$(this).dialog("close");'
            else:
                tmp += 'window.location.replace("%(action)s")'
            tmp += '}'
            buttons_js.append (tmp%(locals()))

        # Add the property
        js = '/* code */ { %s }' %(','.join(buttons_js))
        return js

    def get_js_to_show (self):
        return " $('#%s').dialog('open');" % (self.id)
