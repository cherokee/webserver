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
from HTTP import HTTP_Redir

HEADERS = [
    '<script type="text/javascript" src="/static/js/jquery.uploadProgress.js"></script>'
]

HTML = """
<style type="text/css">
.bar {
  width: 300px;
}

#progress {
  background: #eee;
  border: 1px solid #222;
  margin-top: 20px;
}

#progressbar {
  width: 0px;
  height: 24px;
  background: #333;
}
</style>

<form id="upload" enctype="multipart/form-data" action="%(upload_url)s" method="post">
   <input name="file" type="file"/>
   <input type="submit" value="Upload"/>
</form>

<div id="uploading">
    <div id="progress" class="bar">
       <div id="progressbar">&nbsp;</div>
    </div>
</div>

<div id="percents"></div>
"""

JS = """
$('form').uploadProgress({
	/* scripts locations for safari */
	jqueryPath:         "/static/js/jquery-1.3.2.js",
	uploadProgressPath: "/static/js/jquery.uploadProgress.js",
        progressUrl:        "/progress/",
	interval:           2000,
	uploading:          function(upload) {$('#percents').html(upload.percents+'&#37;');}
});
"""

class MyFieldStorage(cgi.FieldStorage):
    def make_file (self, binary=None):
        return open (os.path.join('/tmp', self.filename), 'wb')

class UploadRequest:
   def __call__ (self, handler):
       scgi = get_scgi()

       print "Writing the post..",
       form = MyFieldStorage (fp=scgi.rfile, environ=scgi.env, keep_blank_values=1)
       print "ok"

       return handler()

class Uploader (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)
        self._url_local = '/uploader_widget_%d' %(self.uniq_id)

        if props:
            self.props = props
        else:
            self.props = {}

        self.id = 'uplodaer%d'%(self.uniq_id)
        handler = self.props.get('handler')

        # Register the uploader path
        publish (self._url_local, UploadRequest, handler=handler)

    def Render (self):
        props = {'id':          self.id,
                 'upload_url':  self._url_local}

        html = HTML %(props)
        js   = JS   %(props)

        render = RenderResponse (html, js, HEADERS)
        return render
