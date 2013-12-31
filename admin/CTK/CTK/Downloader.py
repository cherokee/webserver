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

import os
import time
import urllib2
import tempfile
import threading

import JS
from Box import Box
from ProgressBar import ProgressBar
from Server import publish, request

JS_UPDATING = """
function update_progress_%(id)s() {
  /* Retrieve status
   */
  info = {};
  $.ajax ({type: 'GET', url: "%(url)s/info", async: false, dataType: "json",
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
      (info.status == 'downloading'))
  {
        $('#%(progressbar_id)s').trigger ( $.extend({'type':'update'}, info));
        window.setTimeout (update_progress_%(id)s, 1000);

  } else if (info.status == 'stopped') {
        $('#%(progressbar_id)s').trigger ({'type':'stopped'});

  } else if (info.status == 'finished') {
        $('#%(progressbar_id)s').trigger ({'type':'finished'});

  } else if (info.status == 'error') {
        $('#%(progressbar_id)s').trigger ({'type':'error'});
  }
}
"""

LAUNCH_DOWNLOAD_JS = """
$.ajax ({type: 'GET', url: "%(url)s/start", async: false,
   success: function (data) {
      window.setTimeout (update_progress_%(id)s, 1000);
   }
});
"""

STOP_DOWNLOAD_JS = """
$.ajax ({type: 'GET', url: "%(url)s/stop", async: false});
"""


# Current downloads
downloads = {}

class DownloadEntry (threading.Thread):
    def __init__ (self, url):
        threading.Thread.__init__ (self)

        self.url         = url
        self.size        = 0
        self.percent     = 0
        self.downloaded  = 0
        self.status      = 'init'
        self.wanna_exit  = False
        self.target_temp = None
        self.target_path = None

    def stop (self):
        self.status     = 'stopped'
        self.wanna_exit = True

    def run (self):
        self.opener  = urllib2.build_opener()
        self.request = urllib2.Request (self.url)

        # HTTP Request
        try:
            self.response = self.opener.open (self.request)
        except Exception, e:
            print e
            self.status = 'error'

        length = self.response.headers.getheaders("Content-Length")
        if length:
            self.size = int(length[0])
        else:
            self.size = -1

        self.downloaded = 0

        self.status      = 'downloading'
        self.time_start  = time.time()

        # Temporal storage
        fd, path = tempfile.mkstemp()
        self.target_temp = os.fdopen (fd, "w+")
        self.target_path = path

        # Read response
        while not self.wanna_exit:
            # I/O
            try:
                chunk = self.response.read(1024)
                if not chunk:
                    # Download finished
                    self.status = 'finished'
                    self.target_temp.flush()
                    self.target_temp.close()
                    break
                self.target_temp.write (chunk)
            except Exception, e:
                print e
                # Error
                self.status = 'error'
                break

            # Stats
            self.downloaded += len(chunk)

            if self.size != -1:
                self.percent = int(self.downloaded * 100 / self.size)
            else:
                self.percent = 50


def DownloadEntry_Factory (url, *args, **kwargs):
    global downloads

    if not url in downloads:
        tmp = DownloadEntry (url, *args, **kwargs)
        downloads[url] = tmp
    return downloads[url]

def DownloadEntry_Exists (url):
    global downloads
    return downloads.has_key (url)

class DownloadReport:
    lock = threading.RLock()

    def __call__ (self, url):
       if request.url.endswith('/info'):
           d = downloads[url]
           return '{"status": "%s", "percent": %d, "size": %d, "downloaded": %d}' %(
               d.status, d.percent, d.size, d.downloaded)

       elif request.url.endswith('/stop'):
           d = downloads[url]
           d.stop()
           return 'ok'

       elif request.url.endswith('/start'):
           self.lock.acquire()

           # Current Downloads
           d = downloads[url]
           if d.status == 'init':
               try:
                   d.start()
               except:
                   self.lock.release()
                   raise

           self.lock.release()
           return 'ok'


class Downloader (Box):
    def __init__ (self, name, url, props={}):
        Box.__init__ (self)
        self.url  = url
        self.name = name
        self.id   = "downloader_%s" %(name)

        # Other GUI components
        self.progressbar = ProgressBar()
        self += self.progressbar

        # Register the uploader path
        self._url_local = "/downloader_%s_report" %(name)
        publish (self._url_local, DownloadReport, url=url)

        download = DownloadEntry_Factory (self.url)

    def Render (self):
        render = Box.Render(self)

        props = {'id':             self.id,
                 'url':            self._url_local,
                 'progressbar_id': self.progressbar.id}

        render.js += JS_UPDATING %(props)
        return render

    def JS_to_start (self):
        props = {'id':  self.id,
                 'url': self._url_local}

        return LAUNCH_DOWNLOAD_JS %(props)

    def JS_to_stop (self):
        props = {'id':  self.id,
                 'url': self._url_local}

        return STOP_DOWNLOAD_JS %(props)
