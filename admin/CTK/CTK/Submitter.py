# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009-2014 Alvaro Lopez Ortega
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
from TextArea import TextArea
from TextField import TextField
from PageCleaner import Uniq_Block
from Server import get_server

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
    """
    Widget to submit HTTP POST requests. This renders all the widgets
    contained in it, and upon edition, will post all the input
    elements contained in it to the specified submit_url. Currently,
    the widgets for input elements are:

      Input widgets (grouped by family):
        Checkbox, CheckCfg, CheckboxText, CheckCfgText
        Combobox, ComboCfg
        DatePicker
        HiddenField
        iPhoneToggle, iPhoneCfg
        Radio, RadioText
        TextArea, TextAreaCfg
        TextField, TextFieldPassword, TextCfg, TextCfgPassword, TextCfgAuto
        ToggleButton, ToggleButtonOnOff

      Example:
        submit  = CTK.Submitter('/apply')
        submit += CTK.HiddenField ('foo_name', 'bar_value')
        submit += CTK.TextField({'name': 'baz'})

      Note: All the fields are sent as soon as any one of them is
      modified, unless they have been instanced with certain class
      properties that modify this behavior. Such classes are:

        noauto:   will not submit the data when field is modified.
        required: all fields with this class must be provided before
                  the submission can take effect.

      Example:
        submit  = CTK.Submitter('/apply')
        submit += CTK.TextField({'name': 'foo', 'class': 'required'})
        submit += CTK.TextField({'name': 'bar', 'class': 'required'})

      To asynchronously send only one field, the appropriate approach
      is to use a submitter for each one.
    """

    def __init__ (self, submit_url):
        Container.__init__ (self)
        self.url = submit_url
        self.id  = "submitter%d" %(self.uniq_id)

        # Secure submit
        srv = get_server()
        if srv.use_sec_submit:
            self.url += '?key=%s' %(srv.sec_submit)

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
    """
    Widget to display a submission button. It will submit the data
    contained in the parent Submitter, unless required input fields
    of such Submitter are not provided. Accepts an optional argument:

    caption: text that should appear in the button. By default,
             "Submit".
    """
    def __init__ (self, caption="Submit"):
        Button.__init__ (self, caption)

    def Render (self):
        render = Button.Render(self)
        render.js += FORCE_SUBMIT_JS %({'id': self.id})
        return render
