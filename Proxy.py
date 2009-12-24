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
from Widget import Widget, RenderResponse
from Server import publish

from httplib import HTTPConnection

HTML = """
<div id="%(id_widget)s"></div>
"""

JAVASCRIPT = """
var response = $.ajax({
   url:    "%(proxy_url)s",
   type:   "GET",
   async:  false
}).responseText;

$('#%(id_widget)s').html(response);
"""


class ProxyRequest:
   def __call__ (self, host, req):
      conn = HTTPConnection (host)
      conn.request ("GET", req)
      r = conn.getresponse()
      return r.read()


class Proxy (Widget):
    def __init__ (self, host, req, props={}):
        Widget.__init__ (self)
        self._props     = props
        self._url_local = '/proxy_widget_%d' %(self.uniq_id)

        if not 'id' in props:
            self._props['id'] = 'widget%d'%(self.uniq_id)

        # Register the proxy path
        publish (self._url_local, ProxyRequest, host=host, req=req)

    def Render (self):
        props = {'id_widget': self._props['id'],
                 'proxy_url': self._url_local}

        html = HTML       %(props)
        js   = JAVASCRIPT %(props)

        return RenderResponse (html, js)
