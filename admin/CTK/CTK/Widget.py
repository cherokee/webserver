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
import string

from consts import *
from util import json_dump
from PageCleaner import Postprocess

SYNC_JS_LOAD_JS = """
if (typeof (__loaded_%(name)s) == 'undefined') {
    $.ajax ({url: '%(url)s', type:'get', dataType:'script', async:false, global:false, cache:true, ifModified:true});
    __loaded_%(name)s = true;
}
"""

class RenderResponse:
    def __init__ (self, html='', js='', headers=[], helps=[]):
        self.html    = html
        self.js      = js
        self.headers = headers[:]
        self.helps   = helps[:]

    def clean_up_headers (self):
        noDupes = []
        [noDupes.append(i) for i in self.headers if not noDupes.count(i)]
        self.headers = noDupes

    def __add__ (self, other):
        assert isinstance(other, RenderResponse)

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
        alpha = string.letters + string.digits

        # Synchronous JS loading..
        # Jquery MUST be loaded for this code to work
        sync_js_load = ""

        for js_header in filter(lambda x: x.startswith('<script'), self.headers):
            url_js  = re.findall (r'src="(.+)"', js_header)[0]
            name_js = ''.join (map (lambda x: ('_',x)[x in alpha], re.findall (r'src=".+/(.+)\.js"', js_header)[0]))
            sync_js_load += SYNC_JS_LOAD_JS %({'name':name_js, 'url':url_js})

        # Render the Javascript block
        if self.js or sync_js_load:
            txt += HTML_JS_ON_READY_BLOCK %(sync_js_load + self.js)

        return Postprocess(txt)

    def toJSON (self):
        tmp = filter (lambda x: x, [x.toJSON() for x in self.helps])
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

    def __init__ (self):
        Widget.widget_uniq_id += 1;
        self.uniq_id = Widget.widget_uniq_id;
        self.id      = "widget_%d" %(Widget.widget_uniq_id)
        self.binds   = []

    def Render (self):
        render = RenderResponse()

        for event, js in self.binds:
            render.js += "$('#%s').bind('%s', function(event){ %s });\n" %(self.id, event, js)

        return render

    # Javacript events
    #
    def bind (self, event, js):
        """Bind events to event handlers. The specified Javascript
        code is executed as soon as the event is triggered"""
        self.binds.append ((event, js))

    def JS_to_trigger (self, event, param=None, selector=None):
        """Return the Javascript code required to trigger specified
        event. Custom parameters and selector can be provided."""
        if not selector:
            selector = "$('#%s')" %(self.id)

        if param:
            return "%s.trigger('%s', %s);" %(selector, event, param)
        else:
            return "%s.trigger('%s');" %(selector, event)

    def JS_to_show (self, how='', selector=None):
        """Return the Javascript code required to show the
        widget. Custom duration and selector can be provided.

        Durations are given in milliseconds; higher values indicate
        slower animations, not faster ones. The strings 'fast' and
        'slow' can be supplied to indicate durations of 200 and 600
        milliseconds, respectively.
        """
        if not selector:
            selector = "$('#%s')" %(self.id)
        return '%s.show(%s);' %(selector, how)

    def JS_to_hide (self, how='', selector=None):
        """Return the Javascript code required to hide the
        widget. Custom duration and selector can be provided.

        Durations are given in milliseconds; higher values indicate
        slower animations, not faster ones. The strings 'fast' and
        'slow' can be supplied to indicate durations of 200 and 600
        milliseconds, respectively.
        """
        if not selector:
            selector = "$('#%s')" %(self.id)
        return '%s.hide(%s);' %(selector, how)
