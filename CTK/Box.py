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

from Widget import Widget
from Container import Container
from consts import HTML_JS_BLOCK
from util import *

HTML = '<div id="%(id)s" %(props)s>%(content)s%(embedded_js)s</div>'

class Box (Container):
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
            render.js   = ''
        else:
            js = ''

        props = {'id':          self.id,
                 'props':       props_to_str (self.props),
                 'content':     render.html,
                 'embedded_js': js}

        render.html = HTML %(props)
        return render
