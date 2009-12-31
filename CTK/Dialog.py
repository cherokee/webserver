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
    '<link type="text/css" href="/static/css/jquery-ui-1.7.2.custom.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/static/js/jquery-ui-1.7.2.custom.min.js"></script>'
]

HTML = """
<div id="%(id)s" title="%(title)s">
%(content)s
</div>
"""

JS = """
$("#%(id)s").dialog();
"""

class Dialog (Container):
    def __init__ (self, props=None):
        Container.__init__ (self)

        if props:
            self.props = props
        else:
            self.props = {}

        self.title = self.props.pop('title', '')
        self.id    = 'dialog%d'%(self.uniq_id)

    def Render (self):
        render_child = Container.Render (self)

        props = {'id':      self.id,
                 'title':   self.title,
                 'content': render_child.html}

        html = HTML %(props)
        js   = JS   %(props)

        render = RenderResponse (html)
        render.js      = render_child.js + js
        render.headers = render_child.headers + HEADERS

        return render

