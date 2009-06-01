from base import *

MAGIC="Alvaro: http://www.alobbs.com/"

DIR  = "229"
FILE = "file"

CONF = """
vserver!1!rule!2290!match = fullpath
vserver!1!rule!2290!match!fullpath!1 = /%s/%s
vserver!1!rule!2290!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "FullPath: simple"
        self.request           = "GET /%s/%s HTTP/1.0\r\n" % (DIR, FILE)
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.forbidden_content = ["/bin/sh", "echo"]
        self.conf              = CONF % (DIR, FILE)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0755, CGI_BASE)

