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

from Container import Container
from util import props_to_str

NOTICE_TYPES = ['information', 'important-information',
                'warning', 'error', 'offline', 'online']

HTML = """
<div id="%(id)s" %(props)s>
%(content)s
</div>
"""

class Notice (Container):
    """
    Widget for notes, basically predefined information boxes. All
    arguments are optional:

       Arguments:

          klass: valid values are 'information',
              'important-information', 'warning', 'error', 'offline',
              'online'. By default, 'information'.

          content: a CTK widget to be added initially to the
              container.

          props: dictionary with properties for the HTML DIV element,
              such as {'class': 'test_class', 'id': 'info-box'}

       Examples:
          notice1  = CTK.Notice()
          notice1 += CTK.RawHTML('<p>This is an information point<p>')

          notice2  = CTK.Notice('warning', CTK.RawHTML('<p>Broken element</p>'))
    """

    def __init__ (self, klass='information', content=None, props={}):
        Container.__init__ (self)

        assert klass in NOTICE_TYPES
        self.props = props.copy()

        if 'class' in self.props:
            self.props['class'] = 'dialog-%s %s'%(klass, self.props['class'])
        else:
            self.props['class'] = 'dialog-%s' %(klass)

        if content:
            self += content

    def Render (self):
        render = Container.Render (self)

        render.html = HTML %({'id':      self.id,
                              'content': render.html,
                              'props':   props_to_str(self.props)})
        return render
