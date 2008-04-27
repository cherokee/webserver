from base import *

DIR       = "header_test1_referer_match"
REFERER   = "example.com"

CONF = """
vserver!default!rule!1580!match = header
vserver!default!rule!1580!match!header = Referer
vserver!default!rule!1580!match!match = .+\.com
vserver!default!rule!1580!handler = cgi
"""

CGI = """#!/bin/sh

echo "just a test"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Dirlist: symlinks"

        self.request           = "GET /%s/test HTTP/1.0\r\n" % (DIR) + \
                                 "Referer: %s\r\n" % (REFERER)
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = "just a test"
        self.forbidden_content = ["/bin/sh", "echo"]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 755, CGI)
