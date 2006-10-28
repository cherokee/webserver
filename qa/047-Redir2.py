from base import *

CONF = """
vserver!default!directory!/redir47!handler = redir
vserver!default!directory!/redir47!handler!url = http://www.cherokee-project.com
vserver!default!directory!/redir47!priority = 470
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Redir to URL with params"

        self.request          = "GET /redir47/something?param=1&param2 HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 301
