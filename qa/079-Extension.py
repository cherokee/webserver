from base import *

DIR       = "extension1"
REQUIRED  = '<a href="http://www.alobbs.com/">Alvaro</a>'
FORBIDDEN = "It shouldn't appear in the text"

CONF = """
vserver!1!rule!790!match = directory
vserver!1!rule!790!match!directory = /%s
vserver!1!rule!790!handler = file

vserver!1!rule!791!match = extensions
vserver!1!rule!791!match!extensions = xyz
vserver!1!rule!791!handler = cgi
""" % (DIR)

CGI_BASE = """#!/bin/sh
# %s
echo "Content-Type: text/plain"
echo
echo '%s'
""" % (FORBIDDEN, REQUIRED)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Custom extensions"
        self.request           = "GET /%s/test.xyz HTTP/1.0\r\n" % (DIR)
        self.expected_error    = 200
        self.expected_content  = REQUIRED
        self.forbidden_content = FORBIDDEN
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test.xyz", 0755, CGI_BASE)


