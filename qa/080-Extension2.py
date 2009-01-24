from base import *

DIR       = "extension2"
FILE      = "test.def080"
REQUIRED  = "This is working! :-)"
FORBIDDEN = "It shouldn't appear in the text"

CONF = """
vserver!1!rule!800!match = directory
vserver!1!rule!800!match!directory = /%s
vserver!1!rule!800!handler = file

vserver!1!rule!801!match = extensions
vserver!1!rule!801!match!extensions = abc080,def080,ghi080
vserver!1!rule!801!handler = cgi
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
        self.name              = "Custom extensions, list"
        self.request           = "GET /%s/%s HTTP/1.0\r\n" % (DIR, FILE)
        self.expected_error    = 200
        self.expected_content  = REQUIRED
        self.forbidden_content = FORBIDDEN
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0755, CGI_BASE)

