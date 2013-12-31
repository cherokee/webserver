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
from Container import Container
from consts import HTML_JS_BLOCK
from util import *

HTML = '<div id="%(id)s" %(props)s>%(content)s%(embedded_js)s</div>'

class Box (Container):
    """
    Widget for the base DIV element. All arguments are optional.

       Arguments:
          props: dictionary with properties for the DIV element, such
              as {'class': 'test', 'display': 'none'}

          content: if provided, it must be a CTK widget

          embed_javascript: True|False. Disabled by default. If
              enabled, Javascript code associated to the widget will
              be rendered as part of the DIV definition instead of
              using a separate Javascript block.

       Examples:
          box1 = CTK.Box()
          box2 = CTK.Box({'class': 'test', 'id': 'test-box'},
                         CTK.RawHTML('This is a test box'))
    """
    def __init__ (self, props={}, content=None, embed_javascript=False):
        Container.__init__ (self)
        self.props = props.copy()
        self.embed_javascript = embed_javascript

        # Object ID
        if 'id' in self.props:
            self.id = self.props.pop('id')

        # Initial value
        if content:
            if isinstance (content, Widget):
                self += content
            elif type(content) in (list, type):
                for o in content:
                    self += o
            else:
                raise TypeError, 'Unknown type: "%s"' %(type(content))

    def Render (self):
        render = Container.Render (self)

        if self.embed_javascript and render.js:
            js = HTML_JS_BLOCK %(render.js)
            render.js = ''
        else:
            js = ''

        props = {'id':          self.id,
                 'props':       props_to_str (self.props),
                 'content':     render.html,
                 'embedded_js': js}

        render.html = HTML %(props)
        return render
