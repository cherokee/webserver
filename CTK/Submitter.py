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
from Widget import Widget
from RawHTML import RawHTML
from Container import Container
from TextField import TextField
from Widget import RenderResponse
from PageCleaner import Uniq_Block

HTML = """
<div id="submitter%(id_widget)d" class="submitter">
 %(html)s
 <div class="notice"></div>
</div>
"""

# Initialization
JS_INIT = """
 %(js)s

  var submit_%(id_widget)d = new Submitter('%(id_widget)d', '%(url)s');
  submit_%(id_widget)d.setup (submit_%(id_widget)d);
"""

# Focus on the first <input> of the page
JS_FOCUS = """
$("input:first").focus();
"""

HEADER = [
    '<script type="text/javascript" src="/CTK/js/Submitter.js"></script>'
]

class Submitter (Container):
    def __init__ (self, submit_url):
        Container.__init__ (self)
        self._url = submit_url

    # Public interface
    #
    def Render (self):
        # Child render
        tmp = Container.Render(self)

        # Own render
        props = {'id_widget': self.uniq_id,
                 'url':       self._url,
                 'html':      tmp.html,
                 'js':        tmp.js}

        my = RenderResponse()

        my.html    = HTML    %(props)
        my.js      = JS_INIT %(props)
        my.js     += Uniq_Block (JS_FOCUS %(props))
        my.headers = HEADER + tmp.headers

        my.clean_up_headers()
        return my



FORCE_SUBMIT_JS = """
$("#%(id)s").click(function() {
    /* Figure the widget number of the Submitter */
    var submitter     = $('#%(id)s').parent('.submitter');
    var submitter_num = submitter.attr('id').replace('submitter','');

    /* Invoke its submit method */
    submit_obj = eval("submit_" + submitter_num);
    submit_obj.submit_form (submit_obj);
});
"""

class SubmitterButton(Widget):
    def __init__ (self, caption="Submit"):
        Widget.__init__ (self)

        self.id      = "button_%d" %(self.uniq_id)
        self.caption = caption

    # Public interface
    #
    def Render (self):
        id      = self.id
        caption = self.caption

        html = '<input id="%(id)s" type="button" value="%(caption)s" />' %(locals())
        js   = FORCE_SUBMIT_JS %(locals())

        return RenderResponse (html, js)

