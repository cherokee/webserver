from base import *

DIR       = "header_test2_referer_match"
REFERER   = "example.159com"
MAGIC     = "Dealing with rule based headers (bis).."

CONF = """
vserver!1!rule!1590!match = header
vserver!1!rule!1590!match!header = Referer
vserver!1!rule!1590!match!match = .+\.159com
vserver!1!rule!1590!handler = file

vserver!1!rule!1591!match = header
vserver!1!rule!1591!match!header = Referer
vserver!1!rule!1591!match!match = .+\.159net
vserver!1!rule!1591!handler = cgi
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo 
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Rule header: match II"

        self.request           = "GET /%s/test HTTP/1.0\r\n" % (DIR) + \
                                 "Referer: %s\r\n" % (REFERER)
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = ["/bin/sh", "echo"]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 0755, CGI)
