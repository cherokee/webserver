from base import *

DIR       = "PathTranslated1"
PATH_INFO = "/this/is/path_info"

CONF = """
vserver!1!rule!2250!match = directory
vserver!1!rule!2250!match!directory = /%s
vserver!1!rule!2250!handler = cgi
""" % (DIR)

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "PATH_TRANSLATED: >${PATH_TRANSLATED}<"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: PATH_TRANSLATED"

        self.request          = "GET /%s/test%s HTTP/1.0\r\n" % (DIR, PATH_INFO)
        self.conf             = CONF
        self.expected_error   = 200

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0755, CGI_BASE)

        pt = www + PATH_INFO
        self.expected_content = "PATH_TRANSLATED: >%s<" % (pt)
