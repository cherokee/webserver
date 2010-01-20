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

import os
import cgi

from Server import publish, get_scgi
from Widget import Widget, RenderResponse


HEADERS = [
    '<link type="text/css" href="/static/css/uploadify.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/static/js/swfobject.js"></script>',
    '<script type="text/javascript" src="/static/js/jquery.uploadify.v2.1.0.min.js"></script>'
]

HTML = """
<div id="fileQueue"></div>
<input type="file" name="uploadify" id="uploadify_%(id)s" />
<p><a href="javascript:jQuery('#uploadify_%(id)s').uploadifyClearQueue()">Cancel All Uploads</a></p>
"""

JS = """
$("#uploadify_%(id)s").uploadify({
	'uploader'       : '/static/flash/uploadify.swf',
	'cancelImg'      : '/static/images/uploadify.cancel.png',
	'script'         : '%(upload_url)s',
	'auto'           : true,
	'multi'          : false
});

//	'folder'         : 'uploads',
//	'queueID'        : 'fileQueue',
"""

class MyFieldStorage(cgi.FieldStorage):
    def make_file (self, binary=None):
        return open (os.path.join('/tmp', self.filename), 'wb')

class UploadRequest:
   def __call__ (self):
       scgi = get_scgi()
       form = MyFieldStorage (fp=scgi.rfile, environ=scgi.env, keep_blank_values=1)
       return "1"

class Uploader (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)
        self._url_local = '/uploader_widget_%d' %(self.uniq_id)

        if props:
            self.props = props
        else:
            self.props = {}

        self.id = 'uplodaer%d'%(self.uniq_id)

        # Register the uploader path
        publish (self._url_local, UploadRequest)

    def Render (self):
        props = {'id':         self.id,
                 'upload_url': self._url_local}

        html = HTML %(props)
        js   = JS   %(props)

        render = RenderResponse (html, js, HEADERS)
        return render
