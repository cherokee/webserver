from base import *

CONF = """
vserver!default!directory!/cgi-bin4!handler = cgi
vserver!default!directory!/cgi-bin4!priority = 180
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "CGI with pathinfo III"

        self.request          = "GET /cgi-bin4/inside/test/test_parameter HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = "PathInfo is /test_parameter"

    def Prepare (self, www):
        self.Mkdir (www, "cgi-bin4/inside")
        self.WriteFile (www, "cgi-bin4/inside/test", 0755,
                        """#!/bin/sh

                        echo "Content-Type: text/plain"
                        echo
                        echo "PathInfo is $PATH_INFO"
                        """)
