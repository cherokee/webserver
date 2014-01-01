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

from consts import *
from Widget import Widget
from Server import publish
from util import props_to_str

HTML = """
<div id="%(id)s" %(props)s>
  %(content)s
</div>
"""

REFRESHABLE_UPDATE_JS = """
$.ajax({
   type:    'GET',
   url:     '%(url)s',
   async:   true,
   success: function(msg){
      %(selector)s.html(msg);
      %(on_success)s
   }
});
"""

def render_plain_html (build_func, **kwargs):
    render = build_func (**kwargs)
    return render.toStr()


class Refreshable (Widget):
    """
    Refreshable container. Can be bound to events to regenerate its
    contents. Providing a unique id property is mandatory. Other
    properties are optional.

    Example:
        class Content (CTK.Box):
            def __init__ (self, refresh):
                CTK.Box.__init__ (self)
                button = CTK.Button ('Refresh')
                button.bind ('click', refresh.JS_to_refresh())

                self += CTK.RawHTML('<p>Rendered on %s.</p>' %(time.ctime()))
                self += button

        refresh = CTK.Refreshable ({'id': 'r1'})
        refresh.register (lambda: Content(refresh).Render())
    """
    def __init__ (self, _props={}):
        Widget.__init__ (self)

        assert 'id' in _props, "Property 'id' must be provided"

        self.props = _props.copy()
        if 'class' in self.props:
            self.props['class'] += ' refreshable'
        else:
            self.props['class'] = 'refreshable'

        self.id         = self.props.pop('id')
        self.url        = "/refreshable/%s" %(self.id)
        self.build_func = None

    def register (self, build_func=None, **kwargs):
        """Register the function used to populate the refreshable
        container"""

        self.build_func = build_func
        self.params     = kwargs

        # Register the public URL
        publish (self.url, render_plain_html, build_func=build_func, **kwargs)

    def Render (self):
        render = self.build_func (**self.params)

        props = {'id':      self.id,
                 'props':   props_to_str(self.props),
                 'content': render.html}

        render.html = HTML %(props)
        return render

    def JS_to_refresh (self, on_success='', selector=None, url=None):
        """Return Javascript snippet required to update the contents
        of the refreshable element."""
        if not selector:
            selector = "$('#%s')" %(self.id)

        if len(on_success) > 0:
             on_success = "$(document).ready(function(){ %s });" % on_success

        props = {'selector':   selector,
                 'url':        url or self.url,
                 'on_success': on_success}
        return REFRESHABLE_UPDATE_JS %(props)


JS_URL_INIT = """
$('#%(id)s').bind('refresh_goto', function(event) {
  $(this).data('url', "%(url)s", event.goto);

  $.ajax ({type: "GET", url: event.goto, async: true,
           success: function (msg){
              $('#%(id)s').html(msg);
           }
         });
});
"""

JS_URL_LOAD = """
$('#%(id)s').data('url', "%(url)s");

if ("%(url)s".length > 0) {
  $.ajax ({type: "GET", url: "%(url)s", async: true,
            success: function (msg){
               $('#%(id)s').html(msg);
            }
         });
}
"""

class RefreshableURL (Widget):
    """
    Refreshable container that loads the contents of the specified
    URL.
    """
    def __init__ (self, url='', _props={}):
        Widget.__init__ (self)

        # Properties
        props = _props.copy()
        if 'class' in props:
            props['class'] += ' refreshable-url'
        else:
            props['class'] = 'refreshable-url'

        if 'id' in props:
            self.id = props.pop('id')

        self.props = props
        self.url   = url

    def Render (self):
        props = {'id':      self.id,
                 'url':     self.url,
                 'props':   props_to_str(self.props),
                 'content': ''}

        render = Widget.Render (self)
        render.html += HTML %(props)
        render.js   += JS_URL_LOAD %(props)
        render.js   += JS_URL_INIT %(props)
        return render

    def JS_to_load (self, url):
        """Return Javascript snippet required to load the given url
        into the RefreshableURL container."""
        props = {'id':  self.id,
                 'url': url}
        return JS_URL_LOAD %(props)
