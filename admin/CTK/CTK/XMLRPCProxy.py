# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010-2014 Alvaro Lopez Ortega
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

import types
import util
from consts import *
from Widget import Widget
from Server import publish, get_scgi

HTML = """
<div id="%(id_widget)s" %(extra_props)s></div>
"""

JAVASCRIPT = """
$.ajax({
   url:     "%(proxy_url)s",
   type:    "GET",
   async:   true,
   success: function(msg){
      $('#%(id_widget)s').html(msg);
   }
});
"""

class ProxyRequest:
    def __call__ (self, xmlrpc_func, format_func, debug):
        try:
            raw = util.to_utf8 (xmlrpc_func ())
            fmt = format_func (raw)
            return fmt
        except:
            if debug:
                util.print_exception()
            return ''

class XMLRPCProxy (Widget):
    def __init__ (self, name, xmlrpc_func, format_func, debug=False, props={}):
        assert type(name) == str

        Widget.__init__ (self)
        self._url_local = '/proxy_widget_%s' %(name)
        self.props      = props.copy()

        # Sanity checks
        assert type(xmlrpc_func) in (types.FunctionType, types.MethodType, types.InstanceType)
        assert type(format_func) in (types.FunctionType, types.MethodType, types.InstanceType)

        # Register the proxy path
        publish (self._url_local, ProxyRequest,
                 xmlrpc_func = xmlrpc_func,
                 format_func = format_func,
                 debug       = debug)

    def Render (self):
       render = Widget.Render(self)

       props = {'id_widget':   self.id,
                'proxy_url':   self._url_local,
                'extra_props': util.props_to_str(self.props)}

       render.html += HTML       %(props)
       render.js   += JAVASCRIPT %(props)
       return render

    def JS_to_refresh (self):
        return JAVASCRIPT %({'id_widget': self.id,
                             'proxy_url': self._url_local})
