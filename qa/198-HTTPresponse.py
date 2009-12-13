from base import *

DIR = "cgi-bin_httpresponse"
MSG = "Page eaten by dog"

CONF = """
vserver!1!rule!1980!match = directory
vserver!1!rule!1980!match!directory = /%s
vserver!1!rule!1980!handler = cgi
vserver!1!rule!1980!handler!error_handler = 0
""" % (DIR)

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo "HTTP/1.0 404 %s"
echo "Foo: bar"
echo
echo "Sorry, file not found."
""" % (MSG)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: HTTP response with reply headers"

        self.request           = "GET /%s/test HTTP/1.0\r\n" % (DIR)
        self.conf              = CONF
        self.expected_error    = 404
        self.forbidden_content = MSG

    def Prepare (self, www):
        self.Mkdir (www, DIR)
        self.WriteFile (www, "%s/test"%(DIR), 0755, CGI_BASE)
