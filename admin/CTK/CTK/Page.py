# -*- coding: utf-8 -*-

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

import re

from consts import *
from Container import Container
from Template import Template
from PageCleaner import Postprocess
from Help import HelpEntry, HelpMenu
from util import formatter

PAGE_TEMPLATE_DEFAULT = """\
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
       "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
 <head>
%(head)s
   <script type="text/javascript" src="/CTK/js/jquery-1.3.2.min.js"></script>
 </head>
 <body%(body_props)s>
%(body)s
 </body>
</html>
"""

PAGE_TEMPLATE_MINI = """\
<html>
 <head>%(head)s</head>
 <body>%(body)s</body>
</html>
"""

HEADERS = [
    '<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />',
    '<link rel="stylesheet" type="text/css" href="/CTK/css/CTK.css" />',
    '<script type="text/javascript" src="/CTK/js/common.js"></script>',
    '<script type="text/javascript" src="/CTK/js/Help.js"></script>',
]

def uniq (seq):
    aggregated = ''
    noDupes    = []

    for line in seq:
        if line.startswith('<script'):
            file = re.findall (r'(src=".+?")', line)[0]
            if not file in aggregated:
                noDupes.append(line)
                aggregated += line

        elif line.startswith('<link'):
            file = re.findall (r'(href=".+?")', line)[0]
            if not file in aggregated:
                noDupes.append(line)
                aggregated += line

        else:
            if not line in aggregated:
                noDupes.append(line)
                aggregated += line

    return noDupes

class Page (Container):
    """
    Base widget for web pages. Typically an instance of this class (or
    a class inheriting from it) is used as top level container to
    which every other CTK widget is added.

    Arguments:
        template: custom CTK Template for the pages
        headers: list of entries for the <head> section.
        helps: parameters for CTK HelpMenu, if any.

    Example:
        page = CTK.Page ()
        page += CTK.RawHTML ('<p>Nothing to see here.</p>')
        return page.Render()
    """
    def __init__ (self, template=None, headers=None, helps=None, **kwargs):
        Container.__init__ (self, **kwargs)
        self.js_header_end = True

        if headers:
            self._headers = HEADERS[:] + headers[:]
        else:
            self._headers = HEADERS[:]

        if template:
            self._template = template
        else:
            self._template = Template (content = PAGE_TEMPLATE_DEFAULT)

        self._helps = []
        for entry in helps or []:
            self._helps.append (HelpEntry (entry[1], entry[0]))

    def AddHeaders (self, headers):
        if type(headers) == list:
            self._headers += headers
        else:
            self._headers.append (headers)

    def Render(self):
        # Get the content render
        render = Container.Render(self)

        # Build the <head> text
        self._headers += render.headers

        if self.js_header_end:
            head = "\n".join (filter (lambda l: not '<script' in l, uniq(self._headers)))
        else:
            head = "\n".join (uniq(self._headers))

        # Helps
        all_helps  = self._helps
        all_helps += render.helps

        render_helps = HelpMenu(all_helps).Render().html

        # Javascript
        js = ''

        if self.js_header_end:
            js += "\n".join (filter (lambda l: '<script' in l, uniq(self._headers)))

        if render.js:
            js += formatter (HTML_JS_ON_READY_BLOCK, render.js)

        # Build the <body>
        body = render.html + render_helps
        if render.js:
            body += js

        # Set up the template
        self._template['head']  = head
        self._template['html']  = render.html
        self._template['js']    = js
        self._template['body']  = body
        self._template['helps'] = render_helps

        if not self._template['body_props']:
            self._template['body_props'] = ''

        txt = self._template.Render()
        return Postprocess (txt)

class PageEmpty (Container):
    """
    Widget for minimalistic web pages. Simplified variant with less
    arguments, but otherwise similar to Page.

    Arguments:
        headers: list of entries for the <head> section.

    Example:
        page = CTK.PageEmpty ()
        page += CTK.RawHTML ('<p>Nothing to see here.</p>')
        return page.Render()
    """
    def __init__ (self, headers=[], **kwargs):
        Container.__init__ (self, **kwargs)

        self.headers  = headers[:]
        self.template = Template (content = PAGE_TEMPLATE_MINI)

    def Render(self):
        # Get the content render
        render = Container.Render(self)

        # Build the <head> text
        self.headers += render.headers
        head = "\n".join (uniq(self.headers))

        self.template['head']  = head
        self.template['body']  = render.html

        # Render
        txt = self.template.Render()
        return Postprocess (txt)
