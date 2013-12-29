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

import JS
from Container import Container
from Proxy import Proxy
from Server import get_scgi
from Image import ImageStock
from Box import Box
from PageCleaner import Uniq_Block
from util import props_to_str, json_dump


HEADERS = [
    '<link type="text/css" href="/CTK/css/CTK.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/CTK/js/jquery-ui-1.7.2.custom.min.js"></script>'
]

DIALOG_HTML = """
<div id="%(id)s" title="%(title)s" %(div_props)s>
%(content)s
</div>
"""

DIALOG_JS = """
var dialog_obj = $("#%(id)s");

/* Initialize */
dialog_obj.dialog (%(dialog_props)s);

/* Apply the Cancel/Close style */
var dlg = dialog_obj.parents(".ui-dialog:first");
var buttons = dlg.find(".ui-dialog-buttonpane button");

buttons.addClass('druid-button');
buttons.filter(':contains("%(cancel_str)s"), :contains("%(close_str)s")').addClass('close-button');
"""

def py2js_dic (d):
    js_pairs = []

    for key in d:
        val = d[key]
        if type(val) in (bool, int, float, list):
             js_pairs.append ("'%s': %s" %(key, json_dump(val)))
        elif type(val) == str:
            if '/* code */' in val:
                js_pairs.append ("'%s': %s" %(key, val))
            else:
                js_pairs.append ("'%s': '%s'" %(key, val))
        else:
            assert False, "Unknown data type"

    return "{%s}" % (", ".join(js_pairs))


class Dialog (Container):
    """
    Widget to render dialogs. The dialogs are initially closed by
    default. The behavior can be modified passing javascript
    properties. Arguments are optional.

       Arguments:

           js_props: dictionary containing javascript properties.

               Property   Values       Default
               modal      True, False  True
               resizable  True, False  False
               autoOpen   True, False  false
               draggable  True, False  False
               position   [x,y]        ['center', 85]

           props: dictionary with properties for the <div> HTML element,
               such as {'title': 'My Dialog', 'width': 500}

       Example:
           dialog = CTK.Dialog ({'title': _('Dialog title'), 'width': 500})
           dialog += CTK.RawHTML ("<p>Hello World!</p>")
           dialog.AddButton (_('Cancel'), "close")
           dialog.AddButton (_('Event'), dialog.JS_to_trigger('custom_event'))

           # Show when button is clicked
           button = CTK.Button('Show')
           button.bind ('click', dialog.JS_to_show())
    """
    def __init__ (self, js_props={}, props={}):
        Container.__init__ (self)

        self.js_props = js_props.copy()
        self.props    = props.copy()
        self.title    = self.js_props.pop('title', '')
        self.buttons  = []
        if 'id' in self.props:
            self.id = self.props.pop('id')
        else:
            self.id = 'dialog%d'%(self.uniq_id)

        # Defaults
        if 'modal' not in self.js_props:
            self.js_props['modal'] = True
        if 'resizable' not in self.js_props:
            self.js_props['resizable'] = False
        if 'autoOpen' not in self.js_props:
            self.js_props['autoOpen'] = False
        if 'draggable' not in self.js_props:
            self.js_props['draggable'] = False
        if 'position' not in self.js_props:
            self.js_props['position'] = ['center', 85]

        # Special cases
        if not self.js_props['autoOpen']:
            if 'class' in self.js_props:
                self.js_props['class'] += ' dialog-hidden'
            else:
                self.js_props['class'] = 'dialog-hidden'

            if 'style' in self.props:
                self.props['style'] += ' display:none;'
            else:
                self.props['style'] = 'display:none;'

    def AddButton (self, caption, action):
        """Add button to the dialog, specifying caption and action to
        perform when clicked. Actions can be 'close' (to close
        dialog), a path starting with '/' (to make that path the
        current window location), or a custom Javascript snippet."""
        self.buttons.append ((caption, action))

    def Render (self):
        # Build buttons
        if self.buttons:
            self.js_props['buttons'] = self._render_buttons_property()

        # Render
        render       = Container.Render (self)
        dialog_props = py2js_dic (self.js_props)
        div_props    = props_to_str (self.props)

        props = {'id':           self.id,
                 'title':        self.title,
                 'content':      render.html,
                 'dialog_props': dialog_props,
                 'div_props':    div_props,
                 'cancel_str':   _("Cancel"),
                 'close_str':    _("Close")}

        html = DIALOG_HTML %(props)
        js   = DIALOG_JS   %(props)

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
        """Return Javascript snippet to open the dialog."""
        return "$('#%s').show().dialog('open');" % (self.id)

    def JS_to_close (self):
        """Return Javascript snippet to close the dialog."""
        return " $('#%s').dialog('close');" % (self.id)

    def JS_to_trigger (self, event, _params=[]):
        """Return Javascript snippet to trigger specified event."""
        props = {'id': self.id, 'event': event}

        tmp = ["type: '%s'"%(event)]
        for p in _params:
            assert type(p) == tuple and len(p) == 2
            tmp.append ('%s:%s' %(p[0],p[1]))

        props['details'] = ','.join(tmp)
        return "$('#%(id)s .submitter').trigger({%(details)s});" %(props)


class DialogProxy (Dialog):
    """
    Dialog that synchronously embedds the contents of the provided
    URL. This is a blocking operation, and the render is not completed
    until the proxy receives all the required data. Accepts an
    optional argument to pass properties to the base Dialog class.
    """
    def __init__ (self, url, props={}):
        Dialog.__init__ (self, props.copy())

        scgi = get_scgi()
        self += Proxy (scgi.env['HTTP_HOST'], url)


class DialogProxyLazy (Dialog):
    """
    Dialog that embedds the contents of the provided URL. Accepts an
    optional argument to pass properties to the base Dialog class. The
    contents of the URL are retrieved asynchronously, so the render is
    not blocked on retrieval.
    """
    def __init__ (self, url, _props={}):
        # Properties
        props = _props.copy()
        Dialog.__init__ (self, props)

        # Widget content
        img = ImageStock('loading', {'class': 'no-see'})

        box = Box({'class': 'dialog-proxy-lazy-loader'})
        box += img
        self += box

        # Event
        self.bind ('dialogopen', "$('#%s').show();" %(img.id) + \
                                 JS.Ajax(url, type = 'GET',
                                         success = '$("#%s").html(data);'%(box.id)))


class Dialog2Buttons (Dialog):
    """
    Simplified dialog with close and custom buttons.

    Arguments:
        props: dialog properties to pass to base Dialog class.

        button_name: caption for custom button.

        button_action: action for custom button. Can be 'close' (to
            close dialog), a path starting with '/' (to make that path
            the current window location), or a custom Javascript
            snippet.
    """
    def __init__ (self, props, button_name, button_action):
        Dialog.__init__ (self, props.copy())
        self.AddButton (_('Close'), "close")
        self.AddButton (button_name, button_action)
