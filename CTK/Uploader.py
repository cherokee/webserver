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
import tempfile

from Server import publish, get_scgi
from Widget import Widget, RenderResponse
from PageCleaner import Uniq_Block


HEADERS = [
    '<script type="text/javascript" src="/CTK/js/jquery.uploadProgress.js"></script>'
]

CSS = """
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
"""

HTML = """
 <form id="%(id)s_form" enctype="multipart/form-data" action="%(upload_url)s" method="post">
   <input name="file" type="file"/>
   <input type="submit" value="Upload"/>
 </form>

 <div id="uploading_%(id)s">
    <div id="progress" class="bar">
       <div id="progressbar">&nbsp;</div>
    </div>
 </div>
 <div id="%(id)s_percents"></div>
"""

JS = """
$('#%(id)s_form').uploadProgress({
	/* scripts locations for safari */
	jqueryPath:         "/CTK/js/jquery-1.3.2.js",
	uploadProgressPath: "/CTK/js/jquery.uploadProgress.js",
        progressBar:        "#uploading_%(id)s #progressbar",
        progressUrl:        "/progress/",
	interval:           2000,
	uploading:          function(upload) {$('#%(id)s_percents').html(upload.percents+'&#37;');},
        start:              function(upload) {$('#%(id)s_form').hide('slow');}
});
"""

# Field Storage classes
#
class FieldStorage_Direct(cgi.FieldStorage):
    def make_file (self, binary=None):
        self.target_path = os.path.join (self.target_dir, self.filename)
        return open (self.target_path, 'w+b')

class FieldStorage_Temporal(cgi.FieldStorage):
    def make_file (self, binary=None):
        return self._file


# Internal Proxy
#
class UploadRequest:
   def __call__ (self, handler, target_dir, params, direct):
       scgi = get_scgi()

       # Beware: The functionality is invoked from the constructor!
       if direct:
           FieldStorage_Direct.target_dir = target_dir
           form = FieldStorage_Direct (fp=scgi.rfile, environ=scgi.env, keep_blank_values=1)
           return handler (form['file'].filename, target_dir,
                           form['file'].filename, params)

       # Upload to a temporal file
       fd, target_path = tempfile.mkstemp (prefix='CTK_upload_', dir=target_dir)

       FieldStorage_Temporal._file = os.fdopen(fd, 'w+b')
       FieldStorage_Temporal._path = target_path

       form = FieldStorage_Temporal (fp=scgi.rfile, environ=scgi.env, keep_blank_values=1)

       return handler (form['file'].filename, target_dir,
                       FieldStorage_Temporal._path, params)


# Uploader CTK widget
#
class Uploader (Widget):
    def __init__ (self, props=None, params=None, direct=True):
        Widget.__init__ (self)
        self._url_local = '/uploader_widget_%d' %(self.uniq_id)

        if props:
            self.props = props
        else:
            self.props = {}

        self.id = 'uplodaer%d'%(self.uniq_id)
        handler    = self.props.get('handler')
        target_dir = self.props.get('target_dir')

        # Register the uploader path
        publish (self._url_local, UploadRequest,
                 handler=handler, target_dir=target_dir, params=params, direct=direct)

    def Render (self):
        props = {'id':         self.id,
                 'upload_url': self._url_local}

        raw_html  = Uniq_Block(CSS)
        raw_html += HTML

        html = raw_html %(props)
        js   = JS       %(props)

        render = RenderResponse (html, js, HEADERS)
        return render
