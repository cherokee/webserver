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
from util import props_to_str

class Button (Widget):
    """
    Widget for the BUTTON HTML element. All arguments are optional.

       Arguments:
          caption: text that should appear in the button. By default,
              "Submit".

          props: dictionary with properties for HTML element, such as
              {'class': 'test', 'display': 'none'}

       Examples:
          button1 = CTK.Button()
          button2 = CTK.Box('Validate', {'class': 'blue_button'})
    """
    def __init__ (self, caption="Submit", props={}):
        Widget.__init__ (self)
        self.props = props.copy()

        if 'class' in props:
            self.props['class'] += " button"
        else:
            self.props['class'] = "button"

        self.id      = props.pop('id', "button_%d"%(self.uniq_id))
        self.caption = caption

    # Public interface
    #
    def Render (self):
        id      = self.id
        caption = self.caption
        props   = props_to_str (self.props)

        html = '<button id="%(id)s" %(props)s>%(caption)s</button>' %(locals())

        render = Widget.Render (self)
        render.html += html

        return render
