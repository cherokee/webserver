from base import *

MAGIC   = "Rule OR: matches"
DIR     = "DirOr1"
FILE    = "test.cgi"

CONF = """
vserver!1!rule!1640!match = directory
vserver!1!rule!1640!match!directory = /%s
vserver!1!rule!1640!handler = file

vserver!1!rule!1641!match = or
vserver!1!rule!1641!match!left = directory
vserver!1!rule!1641!match!left!directory = /%s
vserver!1!rule!1641!match!right = extensions
vserver!1!rule!1641!match!right!extensions = not_cgi,other
vserver!1!rule!1641!handler = cgi
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule or: match"

        self.request           = "GET /%s/%s HTTP/1.0\r\n" % (DIR, FILE)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]
        self.conf              = CONF % (DIR, DIR)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, FILE, 0755, CGI)
