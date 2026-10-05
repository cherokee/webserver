from qa_base import *

MAGIC = "I_bet_Cherokee_is_better"
URL   = "http://www.example.com"

CONF = """
vserver!1!rule!1180!match = request
vserver!1!rule!1180!match!request = ^/req_redir_compact1/(.*)/
vserver!1!rule!1180!handler = redir
vserver!1!rule!1180!handler!rewrite!1!show = 1
vserver!1!rule!1180!handler!rewrite!1!substring = %s/$1
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Request Redir compact"
        self.request           = f"GET /req_redir_compact1/{MAGIC}/ HTTP/1.0\r\n"
        self.conf              = CONF % (URL)
        self.expected_error    = 301
        self.expected_content  = f"{URL}/{MAGIC}"
