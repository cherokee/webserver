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

from Widget import Widget
from util import props_to_str
from Image import ImageStock

HTML = """
<div id="%(id)s" %(props)s>
  %(on_html)s
  %(off_html)s
  <input id="hidden_%(id)s" name="%(id)s" type="hidden" value="%(value)s" />
</div>
"""

JS = """
/* On click
 */
$('#%(id)s').click (function() {
   var self   = $(this);
   var hidden = self.find ('input:hidden');
   var val    = hidden.val();

   if (val == "0") {
       hidden.val("1");
       self.find('#%(on_id)s').show();
       self.find('#%(off_id)s').hide();
   } else {
       hidden.val("0");
       self.find('#%(off_id)s').show();
       self.find('#%(on_id)s').hide();
   }

   self.trigger({'type': "changed", 'value': val});
   return false;
});

/* Init
 */
var self = $('#%(id)s');
if ("%(value)s" == "1") {
   self.find('#%(on_id)s').show();
   self.find('#%(off_id)s').hide();
} else {
   self.find('#%(off_id)s').show();
   self.find('#%(on_id)s').hide();
}
"""


class ToggleButton (Widget):
    """
    Widget that toggles between two states on-click.

       Arguments:
           on:  CTK widget to display when ToggleButton is active.
           off: CTK widget to display when ToggleButton is inactive.
           active: Boolean to indicate initial state. Defaults to
               True.
           props: dictionary with optional properties for the HTML
               element, such as {'name': 'foo', 'id': 'bar'}. Defaults
               to {}

       Examples:
          toggle = CTK.ToggleButton (CTK.RawHTML('On'),
                                     CTK.RawHTML('Off'),
                                     active=False)
    """
    def __init__ (self, on, off, active=True, props={}):
        Widget.__init__ (self)

        assert isinstance(on,  Widget)
        assert isinstance(off, Widget)

        self.props      = props.copy()
        self.active     = active
        self.widget_on  = on
        self.widget_off = off

        if 'class' in props:
            self.props['class'] += " togglebutton"
        else:
            self.props['class'] = "togglebutton"

        self.id = props.pop('id', "togglebutton_%d"%(self.uniq_id))

    # Public interface
    #
    def Render (self):
        id      = self.id
        props   = props_to_str (self.props)
        on_id   = self.widget_on.id
        off_id  = self.widget_off.id
        value   = "01"[int(self.active)]

        # Render embedded widgets
        render_on  = self.widget_on.Render()
        render_off = self.widget_off.Render()
        on_html  = render_on.html
        off_html = render_off.html

        # Render
        render = Widget.Render (self)
        render.html += HTML %(locals())
        render.js   += JS   %(locals())

        # Merge the image renders, just in case
        render_on.html  = ''
        render_off.html = ''
        render += render_on
        render += render_off

        return render

class ToggleButtonOnOff (ToggleButton):
    """
    Widget that toggles between ON and OFF two states
    on-click. Arguments are optional.

       Arguments:
           active: Boolean to indicate initial state. Defaults to
               True.

           props: dictionary with optional properties for the HTML
               element, such as {'name': 'foo', 'id': 'bar'}. Defaults
               to {}

       Examples:
          toggle = CTK.ToggleButtonOnOff (CTK.RawHTML('On'),
                                          CTK.RawHTML('Off'),
                                          active=False)
    """
    def __init__ (self, active=True, props={}):
        ToggleButton.__init__ (self,
                               ImageStock('on',  {'title': _("Disable")}),
                               ImageStock('off', {'title': _("Enable")}),
                               active, props.copy())

