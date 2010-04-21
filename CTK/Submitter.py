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
from Button import Button
from RawHTML import RawHTML
from Container import Container
from TextField import TextField
from PageCleaner import Uniq_Block

HEADER = ['<script type="text/javascript" src="/CTK/js/Submitter.js"></script>']

# Placeholder
HTML = """
<div id="%(id)s" class="submitter">
 %(content)s
 <div class="notice"></div>
</div>
"""

# Initialization
JS_INIT = """
  $("#%(id)s").Submitter ('%(url)s', '%(optional)s')
          .bind('submit', function() {
                $(this).data('submitter').submit_form();
          });
"""

# Focus on the first <input> of the page
JS_FOCUS = """
  if ($("input:first").hasClass('filter')) {
      $("input:first").next().focus();
  } else {
      $("input:first").focus();
  }
  $("#activity").hide();
"""

class Submitter (Container):
    def __init__ (self, submit_url):
        Container.__init__ (self)
        self.url = submit_url
        self.id  = "submitter%d" %(self.uniq_id)

    def Render (self):
        # Child render
        render = Container.Render(self)

        # Own render
        props = {'id':       self.id,
                 'id_uniq':  self.uniq_id,
                 'url':      self.url,
                 'content':  render.html,
                 'optional': _('Optional')}

        js = JS_INIT %(props) + Uniq_Block (JS_FOCUS %(props))

        render.html     = HTML %(props)
        render.js       = js + render.js
        render.headers += HEADER

        render.clean_up_headers()
        return render

    def JS_to_submit (self):
        return "$('#%s').data('submitter').submit_form();" %(self.id)
        #return "submit_%d.submit_form (submit_%d);" % (self.uniq_id, self.uniq_id)


FORCE_SUBMIT_JS = """
$("#%(id)s").click(function() {
    /* Figure the widget number of the Submitter */
    var submitter = $(this).parents('.submitter');
    $(submitter).data('submitter').submit_form();
});
"""

class SubmitterButton (Button):
    def __init__ (self, caption="Submit"):
        Button.__init__ (self, caption)

    def Render (self):
        render = Button.Render(self)
        render.js += FORCE_SUBMIT_JS %({'id': self.id})
        return render
