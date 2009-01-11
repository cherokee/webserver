from base import *

MAGIC = "This is the magic string"
CONF = """
vserver!1!rule!150!match = directory
vserver!1!rule!150!match!directory = /cgi-bin1
vserver!1!rule!150!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI Execution"

        self.request          = "GET /cgi-bin1/test HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "cgi-bin1")
        self.WriteFile (www, "cgi-bin1/test", 0755, CGI_BASE)
