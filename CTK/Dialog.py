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

import JS
from Container import Container
from Proxy import Proxy
from Server import get_scgi
from Image import ImageStock
from Box import Box
from PageCleaner import Uniq_Block


HEADERS = [
    '<link type="text/css" href="/CTK/css/CTK.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/CTK/js/jquery-ui-1.7.2.custom.min.js"></script>'
]

DIALOG_HTML = """
<div id="%(id)s" title="%(title)s">
%(content)s
</div>
"""

DIALOG_JS = """
$("#%(id)s").dialog (%(dialog_props)s);
"""

DIALOG_BUTTON_JS = """
$("#%(id)s").bind ('dialogopen', function(event, ui){
   $('.ui-dialog-buttonpane :button').each (function(){
       var html = $(this).html();
         if (! html || html.indexOf('<span') != 0) {
            $(this).wrapInner('<span class=\"button-outter\"><span class=\"button-inner\"></span></span>');
         }
   });
});
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
        if 'modal' not in self.props:
            self.props['modal'] = True
        if 'resizable' not in self.props:
            self.props['resizable'] = False
        if 'autoOpen' not in self.props:
            self.props['autoOpen'] = False
        if 'draggable' not in self.props:
            self.props['draggable'] = False

    def AddButton (self, caption, action):
        self.buttons.append ((caption, action))

    def Render (self):
        # Build buttons
        if self.buttons:
            self.props['buttons'] = self._render_buttons_property()

        # Render
        render       = Container.Render (self)
        dialog_props = py2js_dic (self.props)

        props = {'id':           self.id,
                 'title':        self.title,
                 'content':      render.html,
                 'dialog_props': dialog_props}

        html  = DIALOG_HTML %(props)
        js    = DIALOG_JS   %(props)
        js   += Uniq_Block (DIALOG_BUTTON_JS %(props))

        render.html     = html
        render.js      += js
        render.headers += HEADERS

        return render

    def _render_buttons_property (self):
        buttons_js = []

        # Render the button entries
        for tmp in self.buttons:
            button_caption, action = tmp
            tmp = '"%(button_caption)s": function() {'
            if action == "close":
                tmp += '$(this).dialog("close");'
            elif action[0] == '/':
                tmp += 'window.location.replace("%(action)s")'
            else:
                tmp += action
            tmp += '}'
            buttons_js.append (tmp%(locals()))

        # Add the property
        js = '/* code */ { %s }' %(','.join(buttons_js))
        return js

    def JS_to_show (self):
        return " $('#%s').dialog('open');" % (self.id)

    def JS_to_close (self):
        return " $('#%s').dialog('close');" % (self.id)

    def JS_to_trigger (self, event):
        return " $('#%s .submitter').trigger('%s');" %(self.id, event)


class DialogProxy (Dialog):
    def __init__ (self, url, props=None):
        Dialog.__init__ (self, props)

        scgi = get_scgi()
        self += Proxy (scgi.env['HTTP_HOST'], url)


class DialogProxyLazy (Dialog):
    def __init__ (self, url, props=None):
        Dialog.__init__ (self, props)

        box = Box()
        box += ImageStock('loading')
        self += box

        self.bind ('dialogopen', JS.Ajax(url,
                                         type    = 'GET',
                                         success = '$("#%s").html(data);'%(box.id)))


class Dialog2Buttons (Dialog):
    def __init__ (self, props, button_name, button_action):
        Dialog.__init__ (self, props)
        self.AddButton (button_name, button_action)
        self.AddButton (_('Close'), "close")
