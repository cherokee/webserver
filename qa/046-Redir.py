from base import *

CONF = """
vserver!1!rule!460!match = directory
vserver!1!rule!460!match!directory = /redir46
vserver!1!rule!460!handler = redir
vserver!1!rule!460!handler!url = http://www.cherokee-project.com
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Redir to URL"

        self.request          = "GET /redir46/ HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 301
