from base import *

CONF = """
vserver!1!rule!470!match = directory
vserver!1!rule!470!match!directory = /redir47
vserver!1!rule!470!handler = redir
vserver!1!rule!470!handler!url = http://www.cherokee-project.com
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Redir to URL with params"

        self.request          = "GET /redir47/something?param=1&param2 HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 301
