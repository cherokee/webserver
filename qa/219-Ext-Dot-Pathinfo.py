from base import *

DIR      = "219_dots_in_pathinfo"
PATHINFO = "/foo.bar/this.thing/a.b.c.d"

CONF = """
vserver!1!rule!2191!match = extensions
vserver!1!rule!2191!match!extensions = a219b,k219l
vserver!1!rule!2191!handler = cgi

vserver!1!rule!2190!match = directory
vserver!1!rule!2190!match!directory = /%s
vserver!1!rule!2190!handler = file
""" % (DIR)

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
echo "PATH_INFO: ($PATH_INFO)"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Extensions: Dots in Pathinfo"
        self.request           = "GET /%s/index.a219b%s HTTP/1.0\r\n" %(DIR, PATHINFO)
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = "PATH_INFO: (%s)" % (PATHINFO)

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "index.a219b", 0755, CGI_BASE)

