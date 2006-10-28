from base import *

CONF = """
vserver!default!directory!/redir46!handler = redir
vserver!default!directory!/redir46!handler!url = http://www.cherokee-project.com
vserver!default!directory!/redir46!priority = 460
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Redir to URL"

        self.request          = "GET /redir46/ HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 301
