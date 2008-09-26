from Page import *
from Table import *
from CherokeeManagement import *

from time import asctime

INET_WARNING = """
A working connection to the Internet is required for this form to work.
"""

CHEROKEE_FEEDBACK_URL = "/cgi-bin/feedback"

REPORT_TEMPLATE = """
=====================================================
Name:  %(name)s <%(email)s>
Date:  %(date)s
=====================================================

Message
-------
%(body)s

Server Information
------------------
%(server)s

Configuration
-------------
%(conf)s
"""

THANKS_TEXT = """
<h1>Report sent</h1>
<p>Thank you a million for your feedback. We do appreciate your help.</p>

%(reply)s

<h2>Report</h2>
<pre>
%(report)s
</pre>
"""

class PageFeedback (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'feedback', cfg)
        FormHelper.__init__ (self, 'feedback', cfg)

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', 'Feedback')
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_content (self):
        txt = '<h1>Feedback</h1>'
        txt += self.Dialog(INET_WARNING, 'important-information')

        table = Table(2)
        self.AddTableEntry (table, 'Name',  'name')
        self.AddTableEntry (table, 'EMail', 'email')
        self.AddTableCheckbox (table, 'Include configuration', 'include_conf', True)
        txt += str(table)

        txt += """<p>Message:</p>
                  <p><textarea name="body" id="body" rows="20" style="width:100%;"></textarea></p>
               """
        form = Form ("/%s" % (self._id),auto=False)
        return form.Render(txt)

    def _send_report (self, text):
        import httplib, urllib

        params  = urllib.urlencode({'report': text})
        headers = {"Content-type": "application/x-www-form-urlencoded",
                   "Accept":       "text/plain"}

        conn = httplib.HTTPConnection("www.cherokee-project.com")
        conn.request("POST", CHEROKEE_FEEDBACK_URL, params, headers)
        response = conn.getresponse()
        try:
            data = response.read()
        except:
            data = 'Error reading response from the serrver'
        conn.close()
        return data
        
    def _op_apply_changes (self, uri, post):
        include_conf = (post.get_val('include_conf') == 'on')
        if include_conf:
            configuration = str(self._cfg)
        else:
            configuration = 'Not included'

        
        
        params = {
            'name':   post.pop('name',  ''),
            'email':  post.pop('email', ''),
            'body':   post.pop('body',  ''),
            'date':   asctime(),
            'conf':   configuration,
            'server': cherokee_get_server_info()
        }

        txt = REPORT_TEMPLATE % params
        reply = self._send_report(txt)

        return self._op_render_thanks(txt, reply)

    def _op_render_thanks (self, text, reply):
        params = {
            'report': text,
            'reply':  self.Indent(reply)
        }

        self.AddMacroContent ('title', 'Feedback: Thank you')
        self.AddMacroContent ('content', THANKS_TEXT % params)
        return Page.Render(self)

