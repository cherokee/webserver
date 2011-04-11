# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

import CTK

import re
import os
import sys
import gzip
import time
import urllib2
import threading

from util import *
from consts import *
from ows_consts import *
from configured import *


global_index      = None
global_index_lock = threading.Lock()

def Index():
    global global_index

    global_index_lock.acquire()

    if not global_index:
        try:
            global_index = Index_Class()
        except:
            global_index_lock.release()
            raise

    global_index_lock.release()
    return global_index


def cached_download (url, return_content=False):
    # Open the connection
    request = urllib2.Request (url)
    opener  = urllib2.build_opener()

    # Cache file
    url_md5 = CTK.util.md5 (url).hexdigest()
    cache_file = os.path.join (CHEROKEE_OWS_DIR, url_md5)

    tmp = url.split('?',1)[0] # Remove params
    ext = tmp.split('.')[-1]  # Extension
    if len(ext) in range(2,5):
        cache_file += '.%s'%(ext)

    # Check previos version
    if os.path.exists (cache_file):
        s = os.stat (cache_file)
        t = time.strftime ("%a, %d %b %Y %H:%M:%S GMT", time.gmtime(s.st_mtime))
        request.add_header ('If-Modified-Since', t)

    # Send request
    try:
        if sys.version_info < (2, 6):
            stream = opener.open (request)
        else:
            stream = opener.open (request, timeout=10)
    except urllib2.HTTPError, e:
        if e.code == 304:
            # 304: Not Modified
            if return_content:
                return open (cache_file, 'rb').read()
            return cache_file
        raise

    # Content
    content = stream.read()

    # Store
    f = open (cache_file, "wb+")
    f.write (content)
    f.close()

    if return_content:
        return open (cache_file, 'rb').read()
    return cache_file


class Index_Class:
    def __init__ (self):
        repo_url = CTK.cfg.get_val ('admin!ows!repository', REPO_MAIN)

        self.url        = os.path.join (repo_url, 'index.py.gz')
        self.content    = {}
        self.local_file = None
        self.error      = False

        # Initialization
        self.Update()

    def Update (self):
        # Download
        try:
            local_file = cached_download (self.url)
        except urllib2.HTTPError, e:
            self.error = True
            return

        # Shortcut: Do not parse if it hasn't changed
        if ((local_file == self.local_file) and
            (os.stat(local_file).st_mtime == os.stat(self.local_file).st_mtime)):
            return
        self.local_file = local_file

        # Read
        f = gzip.open (local_file, 'rb')
        content = f.read()
        f.close()

        # Parse
        self.content = CTK.util.data_eval (content)

    def __getitem__ (self, k):
        return self.content[k]

    def get (self, k, default=None):
        return self.content.get (k, default)

    # Helpers
    #
    def get_package (self, pkg, prop=None):
        pkg = self.content.get('packages',{}).get(pkg)
        if pkg and prop:
            return pkg.get(prop)
        return pkg

    def get_package_list (self):
        return self.content.get('packages',{}).keys()


if __name__ == "__main__":
    i = Index()
    i.Download()
    i.Parse()

    print i['build_date']
    print i.get_package_list()
    print i.get_package('phpbb')
    print i.get_package('phpbb', 'installation')
