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

from __future__ import annotations

import re
import string
from typing import Any, Optional

from .consts import *
from .util import json_dump
from .PageCleaner import Postprocess
from functools import reduce

SYNC_JS_LOAD_JS = """
if (typeof (__loaded_%(name)s) == 'undefined') {
    $.ajax ({url: '%(url)s', type:'get', dataType:'script', async:false, global:false, cache:true, ifModified:true});
    __loaded_%(name)s = true;
}
"""

class RenderResponse:
    def __init__ (self, html: str = '', js: str = '',
                  headers: Optional[list[str]] = None,
                  helps: Optional[list[Any]] = None):
        self.html    = html
        self.js      = js
        self.headers = list(headers or [])
        self.helps   = list(helps or [])

    def clean_up_headers (self):
        noDupes = []
        [noDupes.append(i) for i in self.headers if not noDupes.count(i)]
        self.headers = noDupes

    def __add__ (self, other: RenderResponse) -> RenderResponse:
        if not isinstance(other, RenderResponse):
            return NotImplemented

        # New response obj
        i = RenderResponse()

        # Append the new response
        i.html    = self.html    + other.html
        i.js      = self.js      + other.js
        i.headers = self.headers + other.headers
        i.helps   = self.helps   + other.helps

        # Sort the headers
        i.clean_up_headers()
        return i

    def toStr (self):
        txt = self.html
        alpha = string.ascii_letters + string.digits

        # Synchronous JS loading..
        # Jquery MUST be loaded for this code to work
        sync_js_load = ""

        for js_header in [x for x in self.headers if x.startswith('<script')]:
            url_match = re.search (r'src="([^"]+)"', js_header)
            if url_match is None:
                continue

            url_js = url_match.group (1)
            name_match = re.search (r'src="(?:[^"]*/)?([^"/]+)\.js(?:["?])', js_header)
            if name_match is None:
                continue

            name_js = ''.join ('_' if x not in alpha else x for x in name_match.group (1))
            sync_js_load += SYNC_JS_LOAD_JS %({'name':name_js, 'url':url_js})

        # Render the Javascript block
        if self.js or sync_js_load:
            txt += HTML_JS_ON_READY_BLOCK %(sync_js_load + self.js)

        return Postprocess(txt)

    def toJSON (self):
        tmp = [x for x in [x.toJSON() for x in self.helps] if x]
        if tmp:
            help = []
            for e in reduce(lambda x,y: x+y, tmp):
                if not e[0] in [x[0] for x in help]:
                    help.append(e)
        else:
            help = []

        return json_dump({'html':    self.html,
                          'js':      Postprocess(self.js),
                          'headers': self.headers,
                          'helps':   help})


class Widget:
    # Class prop
    widget_uniq_id = 1

    def __init__ (self) -> None:
        Widget.widget_uniq_id += 1;
        self.uniq_id = Widget.widget_uniq_id;
        self.id      = f"widget_{int(Widget.widget_uniq_id)}"
        self.binds: list[tuple[str, str]] = []

    def Render (self) -> RenderResponse:
        render = RenderResponse()

        for event, js in self.binds:
            render.js += "$('#{}').bind('{}', function(event){{ {} }});\n".format(self.id, event, js)

        return render

    # Javacript events
    #
    def bind (self, event: str, js: str) -> None:
        """Bind events to event handlers. The specified Javascript
        code is executed as soon as the event is triggered"""
        self.binds.append ((event, js))

    def JS_to_trigger (self, event: str, param: Optional[str] = None,
                       selector: Optional[str] = None) -> str:
        """Return the Javascript code required to trigger specified
        event. Custom parameters and selector can be provided."""
        if not selector:
            selector = f"$('#{self.id}')"

        if param:
            return f"{selector}.trigger('{event}', {param});"
        else:
            return f"{selector}.trigger('{event}');"

    def JS_to_show (self, how: str | int = '', selector: Optional[str] = None) -> str:
        """Return the Javascript code required to show the
        widget. Custom duration and selector can be provided.

        Durations are given in milliseconds; higher values indicate
        slower animations, not faster ones. The strings 'fast' and
        'slow' can be supplied to indicate durations of 200 and 600
        milliseconds, respectively.
        """
        if not selector:
            selector = f"$('#{self.id}')"
        return f'{selector}.show({how});'

    def JS_to_hide (self, how: str | int = '', selector: Optional[str] = None) -> str:
        """Return the Javascript code required to hide the
        widget. Custom duration and selector can be provided.

        Durations are given in milliseconds; higher values indicate
        slower animations, not faster ones. The strings 'fast' and
        'slow' can be supplied to indicate durations of 200 and 600
        milliseconds, respectively.
        """
        if not selector:
            selector = f"$('#{self.id}')"
        return f'{selector}.hide({how});'
