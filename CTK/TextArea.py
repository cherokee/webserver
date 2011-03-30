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
from util import props_to_str

HTML = '<textarea id="%(id)s" %(props)s>%(content)s</textarea>'

class TextArea (Widget):
    """
    Widget for <textarea> element. Arguments are optional.

       Arguments:
           props: dictionary with properties for the HTML element,
               such as {'name': 'foo', 'id': 'bar', 'class': 'baz'}.
           content: text initially contained in the textarea

       Examples:
          area = CTK.TextArea({'name': 'report'}, 'Report your problem')
    """

    def __init__ (self, props=None, content=''):
        Widget.__init__ (self)
        self.props   = props
        self.content = content

        if 'id' in props:
            self.id = self.props.pop('id')

    def Render (self):
        render = Widget.Render (self)

        props = props_to_str (self.props)
        render.html += HTML %({'id':      self.id,
                               'content': self.content,
                               'props':   props})

        return render
