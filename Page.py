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
from Template import Template

DEFAULT_PAGE_TEMPLATE = """\
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
       "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html>
 <head>
%(head)s
 </head>
 <body%(body_props)s>
%(body)s
 </body>
</html>
"""


class Page (Container):
    def __init__ (self, template=None):
        Container.__init__ (self)
        self._headers  = []
        if template:
            self._template = Template (filename = template)
        else:
            self._template = Template (content = DEFAULT_PAGE_TEMPLATE)

    def AddHeaders (self, headers):
        if type(headers) == list:
            self._headers += headers
        else:
            self._headers.append (headers)

    def Render(self):
        body       = Container.Render(self)
        body_props = ''
        head       = "\n".join (self._headers)

        return self._template.Render()
