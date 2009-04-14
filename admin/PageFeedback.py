from Page import *
from Table import *
from CherokeeManagement import *

from time import asctime

# For gettext
N_ = lambda x: x

INET_WARNING = N_('A working connection to the Internet is required for this form to work.')

CHEROKEE_FEEDBACK_URL = "/cgi-bin/feedback"

REPORT_TEMPLATE = """
=====================================================
Name:  {{name}} <{{email}}>
Date:  {{date}}
=====================================================

Message
-------
{{body}}

Server Information
------------------
{{server}}

Configuration
-------------
{{conf}}
"""

THANKS_TEXT = "<h1>%s</h1>\n<p>%s</p>\n\n{{reply}}\n\n<h2>%s</h2><pre>{{report}}</pre>" % \
       (N_('Report sent'),
        N_('Thank you a million for your feedback. We do appreciate your help.'),
        N_('Report'))

class PageFeedback (PageMenu, FormHelper):
    def __init__ (self, cfg):
        PageMenu.__init__ (self, 'feedback', cfg)
        FormHelper.__init__ (self, 'feedback', cfg)

    def _op_render (self):
        content = self._render_content()
        self.AddMacroContent ('title', _('Feedback'))
        self.AddMacroContent ('content', content)
        return Page.Render(self)

    def _render_content (self):
        txt = '<h1>%s</h1>' % (_('Feedback'))
        txt += self.Dialog(INET_WARNING, 'important-information')

        table = Table(2)
        self.AddTableEntry (table, _('Name'),  'name')
        self.AddTableEntry (table, _('EMail'), 'email')
        self.AddTableCheckbox (table, _('Include configuration'), 'include_conf', True)
        txt += str(table)

        txt += "<p>%s</p>" % (_('Message:')) + \
               '<p><textarea name="body" id="body" rows="20" style="width:100%;"></textarea></p>'
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
            data = _('Error reading response from the server')
        conn.close()
        return data

    def _op_apply_changes (self, uri, post):
        include_conf = (post.get_val('include_conf') == 'on')
        if include_conf:
            configuration = str(self._cfg)
        else:
            configuration = _('Not included')



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

        self.AddMacroContent ('title', _('Feedback: Thank you'))
        self.AddMacroContent ('content', THANKS_TEXT % params)
        return Page.Render(self)

