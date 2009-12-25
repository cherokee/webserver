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
from Table import Table
from RawHTML import RawHTML
from Container import Container
from TextField import TextField
from Widget import RenderResponse

BLOCK_HTML = """
<div id="submitter%(id_widget)d">
 %(html)s
 <div id="notice"></div>
</div>
"""

BLOCK_JS = """
 %(js)s

  submit_%(id_widget)d = new Submitter('%(id_widget)d', '%(url)s');
  submit_%(id_widget)d.setup (submit_%(id_widget)d);
"""

HEADER = [
    '<script type="text/javascript" src="/static/js/Submitter.js"></script>'
]

class Submitter (Container):
    def __init__ (self, submit_url):
        Container.__init__ (self)
        self._url = submit_url

    # Public interface
    #
    def Render (self):
        # Child render
        tmp = RenderResponse()

        for widget in self.child:
            tmp += widget.Render()

        # Own render
        props = {'id_widget': self.uniq_id,
                 'url':       self._url,
                 'html':      tmp.html,
                 'js':        tmp.js}

        my = RenderResponse()

        my.html    = BLOCK_HTML %(props)
        my.js      = BLOCK_JS   %(props)
        my.headers = HEADER + tmp.headers

        my.clean_up_headers()
        return my

