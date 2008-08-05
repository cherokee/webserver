from base import *

MAGIC   = "Rule AND: matches"
DIR     = "DirAnd1"
FILE    = "test.cgi"

CONF = """
vserver!1!rule!1620!match = directory
vserver!1!rule!1620!match!directory = /%s
vserver!1!rule!1620!handler = file

vserver!1!rule!1621!match = and
vserver!1!rule!1621!match!left = directory
vserver!1!rule!1621!match!left!directory = /%s
vserver!1!rule!1621!match!right = extensions
vserver!1!rule!1621!match!right!extensions = cgi
vserver!1!rule!1621!handler = cgi
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo 
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Rule and: match"

        self.request           = "GET /%s/%s HTTP/1.0\r\n" % (DIR, FILE) 
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]
        self.conf              = CONF % (DIR, DIR)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE, 0755, CGI)
