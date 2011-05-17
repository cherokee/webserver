from base import *

MAGIC = "This is the magic string"

DIR  = "cgi-bin-perms1"
CONF = """
vserver!1!rule!2860!match = directory
vserver!1!rule!2860!match!directory = /%(DIR)s
vserver!1!rule!2860!handler = cgi
"""%(globals())

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI Wrong permissions"

        self.request        = "GET /%s/test HTTP/1.0\r\n" %(DIR)
        self.conf           = CONF
        self.expected_error = 403

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0400, CGI_BASE)
