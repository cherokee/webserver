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
from util import formatter, props_to_str

LINK_HTML      = '<a id="%(id)s" %(href)s %(props)s>%(content)s</a>'
LINK_ICON_HTML = '<div id="%(id)s"><span class="ui-icon ui-icon-%(icon)s"></span>%(link)s</div>'


class Link (Container):
    """
    Widget for link HTML elements (<a> tag). All arguments are optional.

       Arguments:
           href: linked resource. Mind that an HTML <a> tag without an
               href does not get selected when applying link styles.

           content: CTK widget to show inside the <a>...</a> block.

           props: dictionary with properties for the HTML element,
               such as {'name': 'foo', 'id': 'bar', 'class': 'baz'}

       Examples:
          link1 = CTK.Link('http://example.com', CTK.RawHTML('Example'))
          link2 = CTK.Link(content=CTK.Image({'src': '/var/www/static/image.png'})
    """
    def __init__ (self, href=None, content=None, props={}):
        Container.__init__ (self)
        if href:
            self.href = href[:]
        else:
            self.href = None
        self.props = props.copy()

        if 'id' in self.props:
            self.id = self.props.pop('id')

        if content:
            self += content

    def Render (self):
        render = Container.Render (self)

        if self.href:
            href = 'href="%s"' %(self.href)
        else:
            href = ''

        props = {'id':      self.id,
                 'href':    href,
                 'props':   props_to_str(self.props),
                 'content': render.html}

        render.html = formatter (LINK_HTML, props)
        return render


class LinkWindow (Link):
    """
    Widget for link HTML elements (<a> tag) to be opened on seprate
    window. All arguments are optional.

       Arguments:
           href: linked resource. Mind that an HTML <a> tag without an
               href does not get selected when applying link styles.

           content: CTK widget to show inside the <a>...</a> block.

           props: dictionary with properties for the HTML element,
               such as {'name': 'foo', 'id': 'bar', 'class': 'baz'}

       Examples:
          link1 = CTK.LinkWindow ('http://example.com', CTK.RawHTML('Example'))
          link2 = CTK.LinkWindow (content=CTK.Image({'src': '/var/www/static/image.png'})
    """
    def __init__ (self, href, content=None, props={}):
        self.props = props.copy()
        props['target'] = '_blank'

        Link.__init__ (self, href, content, props)


class LinkIcon (Link):
    def __init__ (self, href="#", icon='newwin', content=None):
        Link.__init__ (self, href)
        self.icon = icon

        if content:
            self += content

    def Render (self):
        render = Link.Render (self)

        props = {'id':   self.id,
                 'icon': self.icon,
                 'link': render.html}

        render.html = formatter (LINK_ICON_HTML, props)
        return render

