from qa_base import *

PATH_INFO = "/param1/param2/param3"

CONF = """
vserver!1!rule!690!match = directory
vserver!1!rule!690!match!directory = /pathinfo3
vserver!1!rule!690!handler = cgi
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "PathInfo, cgi"

        self.request           = f"GET /pathinfo3/test{PATH_INFO} HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = "PathInfo is: "+PATH_INFO

    def Prepare (self, www):
        self.Mkdir (www, "pathinfo3")
        self.WriteFile (www, "pathinfo3/test", 0o555,
                        """#!/bin/sh

                        echo "Content-type: text/html"
                        echo ""
                        echo "PathInfo is: $PATH_INFO"
                        """)
