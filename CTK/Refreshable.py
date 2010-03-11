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

from consts import *
from Widget import Widget
from Server import publish
from PageCleaner import _remove_dupped_code


HTML = '<div id="%s">%s</div>'

REFRESHABLE_UPDATE_JS = """
$.ajax({
   url:     "%(url)s",
   type:    "GET",
   async:   true,
   success: function(msg){
      $('#%(id)s').html(msg);
   }
});
"""

def render_plain_html (build_func, **kwargs):
    render = build_func (**kwargs)

    output = render.html
    if render.js:
        output += _remove_dupped_code(HTML_JS_ON_READY_BLOCK %(render.js))

    return output


class Refreshable (Widget):
    def __init__ (self, _props={}):
        assert 'id' in _props, "'id' is a required property"

        Widget.__init__ (self)
        props = _props.copy()

        self.id  = props.pop('id')
        self.url = "/refreshable/%s" %(self.id)

    def register (self, build_func=None, **kwargs):
        self.build_func = build_func
        self.params     = kwargs

        # Register the public URL
        publish (self.url, render_plain_html, build_func=build_func, **kwargs)

    def Render (self):
        render = self.build_func (**self.params)
        render.html = HTML%(self.id, render.html)
        return render

    def JS_to_refresh (self):
        return REFRESHABLE_UPDATE_JS %({'url':self.url, 'id':self.id})
