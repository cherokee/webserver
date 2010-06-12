# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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
import time
import urllib2
import tempfile
from threading import Thread

import JS
from Box import Box
from ProgressBar import ProgressBar
from Server import publish

JS_UPDATING = """
function update_progress_%(id)s() {
  /* Retrieve status
   */
  info = {};
  $.ajax ({type: 'GET', url: "%(url)s", async: false, dataType: "json",
           success: function (data) {
               for (i in data) {
                 info[i] = data[i];
               }

               if (info.status == 'init') {
                 $('#%(progressbar_id)s').progressbar ('option', 'value', 0);
               } else if ((info.status == 'downloading') ||
                          (info.status == 'finished')) {
                 $('#%(progressbar_id)s').progressbar ('option', 'value', info.percent);
               }
           }
          });

  /* Next step
   */
  if ((info.status == 'init') ||
      (info.status == 'downloading')) {
        window.setTimeout (update_progress_%(id)s, 1000);

  } else if (info.status == 'finished') {
        $('#%(progressbar_id)s').trigger ({'type':'finished'});
  }
}
window.setTimeout (update_progress_%(id)s, 1000);
"""

# Current downloads
downloads = {}


class DownloadEntry (Thread):
    def __init__ (self, url):
        Thread.__init__ (self)

        self.url        = url
        self.size       = 0
        self.percent    = 0
        self.downloaded = 0
        self.status     = 'init'

    def run (self):
        self.opener  = urllib2.build_opener()
        self.request = urllib2.Request (self.url)

        # HTTP Request
        self.response = self.opener.open (self.request)
        self.size = int(self.response.headers.getheaders("Content-Length")[0])
        self.downloaded = 0

        self.status      = 'downloading'
        self.time_start  = time.time()
        self.target_temp = tempfile.TemporaryFile()

        # Read response
        while True:
            # I/O
            chunk = self.response.read(1024)
            print "read len %d" %(len(chunk))
            if not chunk:
                break
            self.target_temp.write (chunk)

            # Stats
            self.downloaded += len(chunk)
            self.percent = int(self.downloaded * 100 / self.size)
            print "Downloading thread", "%d%%" %(self.percent)

        # Download finished
        self.status = 'finished'


def DownloadEntry_Factory (url, *args, **kwargs):
    global downloads

    if not url in downloads:
        tmp = DownloadEntry (url, *args, **kwargs)
        downloads[url] = tmp
        tmp.start()
    return downloads[url]


class DownloadReport:
   def __call__ (self, url):
       d = downloads[url]

       return '{"status": "%s", "percent": %d, "size": %d, "downloaded": %d}' %(
           d.status, d.percent, d.size, d.downloaded)


class Downloader (Box):
    def __init__ (self, url, props={}):
        Box.__init__ (self)
        self.url = url
        self.id  = "downloader_%d" %(self.uniq_id)

        # Current Downloads
        download = DownloadEntry_Factory (self.url)

        # Register the uploader path
        self._url_local = "/downloader_%d_report" %(self.uniq_id)
        publish (self._url_local, DownloadReport, url=url)

        # Other GUI components
        self.progressbar = ProgressBar()
        self += self.progressbar


    def Render (self):
        render = Box.Render(self)

        props = {'id':             self.id,
                 'url':            self._url_local,
                 'progressbar_id': self.progressbar.id}

        render.js += JS_UPDATING %(props)
        return render


